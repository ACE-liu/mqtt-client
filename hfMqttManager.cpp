#include "hfMqttManager.h"
#include "iostream"
#include <string.h>
#include <unistd.h>
#include <vector>
#include <functional>
#include "hfHttpClient.h"
#include "jsoncpp/json.h"
#include "MD5.h"
#include <algorithm>

using namespace std;
// using namespace Json;

namespace { 
    const std::string accessKey = "shimaoec642273e1464e01";
    const std::string secretKey = "7bd26f2936af4b10b3cae8cdc821f97e";
    const int MQTT_QUEUE_ALARM_SIZE = 400;
    const int ROOM_INFO_CALLBACK_TIME = 10000;  //ms 房间信息回调时间
    const int UPLOAD_MSG_TIME_OUT = 5000;    // ms MQTT上传消息确认超时时间

}


static void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
}

static std::string getRandomStr(const int length)
{
    std::string rtnCallId;
    int i, index;
    for(i = 0; i<length; i++)
    {
        index = rand()%36;
        if(index <= 9)
           rtnCallId += '0' + index;
        else
        {
            rtnCallId += 'a' + index - 10;
        }
        
    }
    return rtnCallId;
}

void hfMqttManager::connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    // if(cause !=NULL)
    //     printf("     cause: %s\n", cause);
    hfMqttManager * mqttManager = (hfMqttManager* )context;
    mqttManager->isConnected = false;
    // if(mqttManager->forceQuit)
    // {
    //     printf("uninit mqtt client, quit!\n");
    //     return;
    // }
    // while(!mqttManager->isConnected)
    // {
    //     if(mqttManager->forceQuit)
    //     {
    //         printf("uninit mqtt client, quit!\n");
    //         return;
    //     }
    //      if(!mqttManager->hfMqttClientInit_())
    //      {
    //          sleep(1);
    //      }
    //      else
    //      {
    //          mqttManager->hfMqttClientReSubscribe();
    //      }
         
    // }
}

int hfMqttManager::msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    hfMqttManager * mqttManager = (hfMqttManager* )context;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n", message->payloadlen, (char*)message->payload);
    mqttManager->dealWithMqttMsgReceived(topicName, (char*)message->payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;   
}

hfMqttManager::hfMqttManager()
{
    isConnected = false;
    forceQuit = false;
    ifGetMqttServerInfo = false;
    ifGetHotelRoomInfo = false;
    getMsgClk = nullptr;
    getSubscrbClk = nullptr;
    _thread = nullptr;
    srand((unsigned)time(0));
}

hfMqttManager::~hfMqttManager()
{
    {
        std::unique_lock<std::mutex> lck(mtx);
        forceQuit = true;
        cv.notify_one();
    }
    if(_thread !=nullptr)
        _thread->join();
    _thread.reset();
    hfMqttClientUninit();
}

void hfMqttManager::hfMqttClientUninit()
{
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);    
    client = NULL;
    isConnected = false;
}

std::string hfMqttManager::getSignature(const std::string & accessKey, const std::string & secretKey)
{
    char stringSignTemp[1024];
    std::string timestampStr = getTimestamp();
    sprintf(stringSignTemp, "AccessKey=%s&Timestamp=%s&SecretKey=%s",accessKey.c_str(),timestampStr.c_str(), secretKey.c_str());
    std::string rtn = toolkit::MD5(stringSignTemp).hexdigest();
    std::transform(rtn.begin(), rtn.end(),rtn.begin(), ::toupper);
    return rtn;
}

bool hfMqttManager::hfMqttManagerInit(const string & deviceCode, 
                                      const std::string &httpMqttServerUrl, 
                                      const std::string &httpRoomInfoUrl,  
                                      getHotelRoomMsgCallback getMsgClk, 
                                      getMqttSubscribeMsgCallback getSubscrbClk)
{
    this->deviceCode = deviceCode;
    this->httpMqttServerUrl = httpMqttServerUrl;
    this->httpRoomInfoUrl = httpRoomInfoUrl;
    this->getMsgClk = getMsgClk;
    this->getSubscrbClk = getSubscrbClk;
    
    char topic[128] ={0};
    sprintf(topic, "Topic:shimao/face/%s/%s/state", HF_CODE, deviceCode.c_str());
    mqttUploadTopic = topic;   //添加发布消息topic
    sprintf(topic, "Topic:shimao/face/%s/%s/command", HF_CODE, deviceCode.c_str());
    subscribeTopicList[topic] = 2; //添加监听topic

    if(getMqttServerMsgByDeviceCode(httpMqttServerUrl, deviceCode, this->mqttServerInfo))
    {
        hfMqttClientInit_(this->mqttServerInfo);
        hfMqttClientSubscribe(topic, 2);
    }
    
    if(getRoomInfoByDeviceCode(httpRoomInfoUrl, deviceCode, this->hotelRoomInfo) && getMsgClk!=nullptr)
        getMsgClk(this->hotelRoomInfo);     
    
    _thread = std::make_shared<thread>([this](){ this->handleEventThread();});
}

inline std::string hfMqttManager::getTimestamp()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);  //该函数在sys/time.h头文件中
    long timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return to_string(timestamp);
}

bool hfMqttManager::hfMqttPublishUploadMsg(const MqttUploadMsg& uploadMsg)
{
    Json::Value val;
    val["reqId"] = uploadMsg.reqId;
    val["msgType"] = "UP";
    val["gwCode"] = uploadMsg.deviceInfo.gwCode;
    val["entityCode"] = uploadMsg.deviceInfo.entityCode;
    val["typeCode"] = uploadMsg.deviceInfo.typeCode;
    val["timestamp"] = uploadMsg.timestamp;
    for(auto & attr: uploadMsg.attrList)
    {
        Json::Value tmp;
        tmp["attributeCode"] = attr.attributeCode;
        tmp["value"] = attr.value;
        val["attributesEntities"].append(tmp);
    }
    if(uploadMsg.attrList.size() == 0)
    {
        Json::Value tmp(Json::arrayValue);
        val["attributesEntities"] = tmp;
    }
    for(auto &passRecord : uploadMsg.passRecordList)
    {
        Json::Value tmp;
        tmp["id"] = passRecord.id;
        tmp["name"] = passRecord.name;
        tmp["passPhoto"] = passRecord.passPhoto;
        tmp["passMode"] = passRecord.passMode;
        tmp["resultType"] = passRecord.resultType;
        tmp["passTime"] = passRecord.passTime;
        tmp["personType"] = passRecord.personType;
        if(!passRecord.code.empty())
            tmp["code"] = passRecord.code;
        if(!passRecord.reservcer2.empty())
            tmp["reservcer2"] = passRecord.reservcer2;
        if(!passRecord.reservcer3.empty())
            tmp["reservcer3"] = passRecord.reservcer3;
        val["passRecords"].append(tmp);
    }
    val["alarmInfo"]["code"] = to_string((int)uploadMsg.alarmInfo.alarmCode);
    val["alarmInfo"]["msg"] = uploadMsg.alarmInfo.msg;
    string msg = val.toStyledString();
    return hfMqttClientPublishMsg(mqttUploadTopic, msg, 2, 0);    
}

void hfMqttManager::dealWithMqttMsgReceived(const std::string& topic, const std::string & msg)
{
    cout<<"at dealWithMqttMsgReceived func."<<endl;
    Json::Reader JsonParser;
    Json::Value tempVal;
    if (!JsonParser.parse(msg, tempVal)) {
        cout<<"parse Mqtt Server Msg http rtn failed."<<endl;
        return;
    }
    string msgType = tempVal["msgType"].asString();     
    string reqId = tempVal["reqId"].asString();    
    if(msgType == "FACE_CMD")  //人脸下发
    {
        Json::Value val;
        if(getSubscrbClk != nullptr)
            getSubscrbClk(topic, msg);
        val["reqId"] = reqId;
        val["code"] = 0;
        val["message"] = "success";
        val["msgType"] = "MSG_ACK";
        string retMsg = val.toStyledString();
        hfMqttClientPublishMsg(mqttUploadTopic, retMsg, 2, 0);    
    }
    else if(msgType == "FACE_STATE_RES")   //记录上传确认
    {
        int code = tempVal["code"].asInt();
        std::unique_lock<std::mutex> lck(mtx);
        for(auto it = mqttHasUploadMsgs.begin(); it != mqttHasUploadMsgs.end(); ++it)
        {
            MqttUploadMsg uploadMsg = it->second;
            if(uploadMsg.reqId == reqId) 
            {
                if(code == 0)
                   mqttHasUploadMsgs.erase(it); 
                else  //上传失败
                {
                    uploadMsg.reqId = getRandomStr(32);
                    mqttUploadMsgs.push(uploadMsg);
                    mqttHasUploadMsgs.erase(it);
                    cv.notify_one();                 
                }
                break;
            }
        }     
    }
    else
    {
        cout<<"get unknown msg type: "<<msgType<<endl;
    }
}

bool hfMqttManager::getDeviceAuthorityInfo(MqttServerInfo & mqttInfo) 
{
    lock_guard<std::mutex> _guard(mtx_info);
    if(ifGetMqttServerInfo)
    {
        mqttInfo.clientId = mqttServerInfo.clientId;
        mqttInfo.serverAddr = mqttServerInfo.serverAddr;
        mqttInfo.port = mqttServerInfo.port;
        mqttInfo.password = mqttServerInfo.password;
    }
    return ifGetMqttServerInfo;
}


bool hfMqttManager::hfMqttPublishPassRecords(AlarmTypeCode alarmType, const std::vector<AttributesEntity>& attrList, const std::vector<PassRecord>& passRecordList)
{
    MqttUploadMsg uploadMsg;
    uploadMsg.reqId = getRandomStr(32);
    uploadMsg.msgType = "UP";
    uploadMsg.deviceInfo = deviceInfo;
    uploadMsg.timestamp = getTimestamp();
    uploadMsg.alarmInfo.alarmCode = alarmType;
    uploadMsg.attrList = attrList;
    uploadMsg.passRecordList = passRecordList;
    std::unique_lock<std::mutex> lck(mtx);
    mqttUploadMsgs.push(uploadMsg);
    cv.notify_one();
    return true;
}

bool hfMqttManager::getHotelRoomInfo(HotelRoomInfo & roomInfo) 
{
    lock_guard<std::mutex> _guard(mtx_info);
    if(ifGetHotelRoomInfo)
    {
        roomInfo.floor = hotelRoomInfo.floor;
        roomInfo.hotelId = hotelRoomInfo.hotelId;
        roomInfo.hotelName = hotelRoomInfo.hotelName;
        roomInfo.hotelEnglishName = hotelRoomInfo.hotelEnglishName;
        roomInfo.hotelIcon = hotelRoomInfo.hotelIcon;
        roomInfo.houseTypeId = hotelRoomInfo.houseTypeId;
        roomInfo.houseTypeName = hotelRoomInfo.houseTypeName;
        roomInfo.roomImgUrl = hotelRoomInfo.roomImgUrl;
        roomInfo.phone = hotelRoomInfo.phone;
        roomInfo.remark = hotelRoomInfo.remark;
        roomInfo.roomId = hotelRoomInfo.roomId;
        roomInfo.roomNum = hotelRoomInfo.roomNum;
    }
    return ifGetHotelRoomInfo;
}

bool hfMqttManager::getMqttServerMsgByDeviceCode(const std::string &httpUrl, 
                                                 const std::string & deviceCode, 
                                                 MqttServerInfo & serverInfo)
{
    string rtn;
    string body;
    std::map<string, string> headers;
    headers["brand"] = HF_CODE;
    headers["signature"] = getSignature(accessKey, secretKey);
    headers["Content-Type"] = "application/json;charset=UTF-8";
    headers["timestamp"] = getTimestamp();

    Json::Value val;
    val["entityCode"] = deviceCode;
    body = val.toStyledString();
    if(!hfHttpClient::httpPostRequest(httpUrl, headers, body, rtn))
        return false;
    cout<<"get Mqtt Server Msg success."<<endl;
    Json::Reader JsonParser;
    Json::Value tempVal;
    if (!JsonParser.parse(rtn, tempVal)) {
        cout<<"parse Mqtt Server Msg http rtn failed."<<endl;
        return false;
    }
    int returnCode = tempVal["retcode"].asInt();
    if(returnCode != CODE_SUCCESS)
    {
        cout <<"http post error with code: "<<returnCode<<endl;
        return false;
    }
    serverInfo.clientId = tempVal["data"]["clientId"].asString();
    serverInfo.serverAddr = tempVal["data"]["serviceAddr"].asString();
    string portStr = tempVal["data"]["servicePort"].asString();
    serverInfo.port = stoi(portStr);
    serverInfo.password = tempVal["data"]["password"].asString();
    ifGetMqttServerInfo = true; 
    return true;
}

bool hfMqttManager::getRoomInfoByDeviceCode(const std::string &httpUrl, 
                             const std::string & deviceCode, 
                             HotelRoomInfo & roomInfo)
{
    string rtn;
    string body;
    std::map<string, string> headers;
    headers["brand"] = HF_CODE;
    headers["signature"] = getSignature(accessKey, secretKey);
    headers["Content-Type"] = "application/json";
    // headers["timestamp"] = getTimestamp();

    Json::Value val;
    val["deviceCode"] = deviceCode;
    body = val.toStyledString();
    if(!hfHttpClient::httpPostRequest(httpUrl, headers, body, rtn))
        return false;
    cout<<"get hotel room info success."<<endl;
    Json::Reader JsonParser;
    Json::Value tempVal;
    if (!JsonParser.parse(rtn, tempVal)) {
        cout<<"parse hotel room info http rtn failed."<<endl;
        return false;
    }
    int returnCode = tempVal["retcode"].asInt();
    if(returnCode != CODE_SUCCESS)
    {
        cout <<"http post error with code: "<<returnCode<<endl;
        return false;
    }
    roomInfo.floor = tempVal["data"]["floor"].asString();
    roomInfo.hotelId = tempVal["data"]["hotelId"].asString();
    roomInfo.hotelName = tempVal["data"]["hotelName"].asString();
    roomInfo.hotelEnglishName = tempVal["data"]["hotelEnglishName"].asString();
    roomInfo.hotelIcon = tempVal["data"]["hotelIcon"].asString();
    roomInfo.houseTypeId = tempVal["data"]["houseTypeId"].asString();
    roomInfo.houseTypeName = tempVal["data"]["houseTypeName"].asString();
    roomInfo.roomImgUrl = tempVal["data"]["roomImgUrl"].asString();
    roomInfo.phone = tempVal["data"]["phone"].asString();
    roomInfo.remark = tempVal["data"]["remark"].asString();
    roomInfo.roomId = tempVal["data"]["roomId"].asString();
    roomInfo.roomNum = tempVal["data"]["roomNum"].asString();
    ifGetHotelRoomInfo = true;
    return true;
}


bool hfMqttManager::hfMqttClientInit_(const MqttServerInfo& serverInfo)
{
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    string conUrl = serverInfo.serverAddr + to_string(serverInfo.port);
    if ((rc = MQTTClient_create(&client, conUrl.c_str(), serverInfo.clientId.c_str(),
        MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to create client, return code %d\n", rc);
        isConnected = false;
        return false;
    }
    
    if ((rc = MQTTClient_setCallbacks(client, this, (MQTTClient_connectionLost*)&hfMqttManager::connlost, 
                                      (MQTTClient_messageArrived*)&hfMqttManager::msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to set callbacks, return code %d\n", rc);
        isConnected = false;
        return false;
    }
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = serverInfo.clientId.c_str();
    conn_opts.password = serverInfo.password.c_str();
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        isConnected = false;
        return false;
    }
    isConnected = true;
    cout<<"success to connect. return code "<<rc<<std::endl;
    return true;
}



bool hfMqttManager::hfMqttClientPublishMsg(const std::string& topic, const std::string& msg, int qos, int retained)
{
    int rc;
    if ((rc = MQTTClient_publish(client, topic.c_str(), msg.length(), msg.c_str(), qos, retained, NULL)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to publish message, return code %d\n", rc);
        return false;
    }

    printf("Success to publish message, return code %d\n", rc);
    return true;
}

bool hfMqttManager::hfMqttClientSubscribe(const string & topic, int qos)
{
    int rc;
    if ((rc = MQTTClient_subscribe(client, topic.c_str(), qos)) != MQTTCLIENT_SUCCESS)
    {
    	printf("Failed to subscribe, return code %d\n", rc);
        return false;
    }
    subscribeTopicList[topic] = qos;
    return true;
}


bool hfMqttManager::hfMqttClientSubscribeMany(const std::map<std::string, int> & topics)
{
    int rc;
    bool ret = true;
    for(auto & var : topics)
    {
        if ((rc = MQTTClient_subscribe(client, var.first.c_str(), var.second)) != MQTTCLIENT_SUCCESS)
        {
            printf("Failed to unsubscribe, return code %d\n", rc);
            ret = false;
        }   
        else
        {
            subscribeTopicList[var.first] = var.second;
        }
        
    }
    return ret;
}


bool hfMqttManager::hfMqttClientUnsubscribe(const string & topic)
{
    int rc;
    if ((rc = MQTTClient_unsubscribe(client, topic.c_str())) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to unsubscribe, return code %d\n", rc);
        return false;
    }   
    subscribeTopicList.erase(topic);
    return true;
}

bool hfMqttManager::hfMqttClientUnsubscribeMany(const std::vector<std::string> & topics)
{
    int rc;
    bool ret = true;
    for(const std::string & topic : topics)
    {
        if ((rc = MQTTClient_unsubscribe(client, topic.c_str())) != MQTTCLIENT_SUCCESS)
        {
            printf("Failed to unsubscribe, return code %d\n", rc);
            ret = false;
        }  
        else
        {
            subscribeTopicList.erase(topic);
        }
    }
    return ret;
}



bool hfMqttManager::hfMqttClientReSubscribe()
{
    return hfMqttClientSubscribeMany(subscribeTopicList);
}


void hfMqttManager::handleEventThread()
{
    struct timeval curTime, lastTime, startTime;
    int minWaitTime, tempVal;
    gettimeofday(&startTime,NULL); 
    gettimeofday(&lastTime,NULL); 
    while(!forceQuit)
    {
        minWaitTime = INT_MAX;
        gettimeofday(&curTime,NULL); 
        if(!isConnected && curTime.tv_sec - startTime.tv_sec >= 15*24*60*60)
        {
            ifGetMqttServerInfo = false; //重新鉴权
        }
        if(!ifGetMqttServerInfo)
        {
            lock_guard<std::mutex> _guard(mtx_info);
            if(getMqttServerMsgByDeviceCode(httpMqttServerUrl, deviceCode, this->mqttServerInfo))
            {
                ifGetMqttServerInfo = true; 
            }
            minWaitTime = 1000;
        }

        if(!isConnected && hfMqttClientInit_(this->mqttServerInfo))
        {
            hfMqttClientReSubscribe();  //订阅之前订阅的topic
        }

        if(!isConnected)
            minWaitTime = std::min(minWaitTime, 1000);

        tempVal  = (curTime.tv_sec * 1000 + curTime.tv_usec / 1000) - (lastTime.tv_sec * 1000 + lastTime.tv_usec / 1000);   
        if(tempVal >= ROOM_INFO_CALLBACK_TIME)
        {
            lock_guard<std::mutex> _guard(mtx_info);
            if(getRoomInfoByDeviceCode(httpRoomInfoUrl, deviceCode,  this->hotelRoomInfo) && getMsgClk!=nullptr)
                getMsgClk(this->hotelRoomInfo);  
            lastTime = curTime;
            minWaitTime = std::min(minWaitTime, ROOM_INFO_CALLBACK_TIME);
        }
        else
            minWaitTime = std::min(minWaitTime, ROOM_INFO_CALLBACK_TIME - tempVal);
        
        std::unique_lock<std::mutex> lck(mtx);
        if(mqttUploadMsgs.size() > 0)  //处理上传MQTT事件
        {
            std::cout<<"deal with one upload msg..."<<std::endl;
            MqttUploadMsg uploadMsg = mqttUploadMsgs.front();
            if(hfMqttPublishUploadMsg(uploadMsg))
            {
                mqttUploadMsgs.pop();      
                gettimeofday(&curTime,NULL);
                uint64_t timestamp = curTime.tv_sec * 1000 + curTime.tv_usec / 1000;                 
                mqttHasUploadMsgs[timestamp] = uploadMsg;  //添加到待删除Map
            }
            minWaitTime = 0;
        }
        else if(mqttHasUploadMsgs.size() > 0)             //处理未回复MQTT消息重发
        {
            for(auto it = mqttHasUploadMsgs.rbegin(); it != mqttHasUploadMsgs.rend(); ++it)
            {
                gettimeofday(&curTime,NULL);
                uint64_t timestamp = curTime.tv_sec * 1000 + curTime.tv_usec / 1000; 
                tempVal = timestamp - it->first;
                if(tempVal >= UPLOAD_MSG_TIME_OUT) //超时
                {
                    MqttUploadMsg uploadMsg = it->second;
                    uploadMsg.reqId = getRandomStr(32);
                    hfMqttPublishUploadMsg(uploadMsg);
                    mqttHasUploadMsgs.erase(it->first);
                    mqttHasUploadMsgs[timestamp] = uploadMsg;
                    minWaitTime = 0;
                    break;
                }
                else
                {
                    minWaitTime = std::min(minWaitTime, UPLOAD_MSG_TIME_OUT - tempVal);
                }
                
            }

        }
        if(minWaitTime > 0 )
        {
            cv.wait_for(lck,std::chrono::milliseconds(minWaitTime));  
        }
        if(mqttUploadMsgs.size()>=MQTT_QUEUE_ALARM_SIZE)
            std::cout<<"mqtt msg queue size alarm: "<<mqttUploadMsgs.size()<<endl;
    }
    cout<<"force quit, stop!"<<endl;
}


