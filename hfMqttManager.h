#ifndef _HF_MQTT_MANAGER_H_
#define _HF_MQTT_MANAGER_H_

#include "MQTTClient.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <sys/time.h>
#include <thread>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "hfTypes.h"



typedef std::function<void(const HotelRoomInfo&)> getHotelRoomMsgCallback;
typedef std::function<void(const std::string, const std::string)> getMqttSubscribeMsgCallback;



class hfMqttManager
{
public:
    typedef std::shared_ptr<hfMqttManager> Ptr;
    hfMqttManager();
    ~hfMqttManager();

    bool hfMqttManagerInit(const std::string & deviceCode, 
                           const std::string &httpMqttServerUrl, 
                           const std::string &httpRoomInfoUrl, 
                           getHotelRoomMsgCallback getMsgClk, 
                           getMqttSubscribeMsgCallback getSubscrbClk);


    bool hfMqttClientPublishMsg(const std::string& topic, const std::string& msg, int qos = 2, int retained = 0);
    bool hfMqttClientSubscribe(const std::string& topic, int qos = 2);
    bool hfMqttClientSubscribeMany(const std::map<std::string, int> & topics);
    bool hfMqttClientUnsubscribe(const  std::string &topic);
    bool hfMqttClientUnsubscribeMany(const std::vector<std::string> & topics);


    bool getDeviceAuthorityInfo(MqttServerInfo & mqttInfo);     //主动获取鉴权接口
    bool getHotelRoomInfo(HotelRoomInfo & roomInfo);   //主动获取房间信息接口
    bool hfMqttPublishPassRecords(AlarmTypeCode alarmType, const std::vector<AttributesEntity>& attrList, const std::vector<PassRecord>& passRecordList); //上报通行记录接口
    

private:
   void hfMqttClientUninit();
   bool hfMqttClientInit_(const MqttServerInfo& serverInfo);
   bool hfMqttClientReSubscribe();

   bool getMqttServerMsgByDeviceCode(const std::string &httpUrl, const std::string & deviceCode, 
                                     const std::string & brand, const std::string & signature, MqttServerInfo & serverInfo);

   bool getRoomInfoByDeviceCode(const std::string &httpUrl, const std::string & deviceCode, 
                                const std::string & brand, const std::string & signature, HotelRoomInfo & roomInfo);

   static void connlost(void *context, char *cause);
   static int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message);
   inline std::string getTimestamp();

   bool hfMqttPublishUploadMsg(const MqttUploadMsg& uploadMsg);

   void dealWithMqttMsgReceived(const std::string& topic, const std::string & msg);
   void handleEventThread();

private:
    std::string deviceCode;
    std::string httpMqttServerUrl;
    std::string httpRoomInfoUrl;
    std::string mqttUploadTopic;
    std::map<std::string, int> subscribeTopicList;
    std::queue<MqttUploadMsg> mqttUploadMsgs;    //待上传数据队列
    std::map<uint64_t, MqttUploadMsg> mqttHasUploadMsgs;  //已上传，待删除数据队列, key为上传时间戳

    getHotelRoomMsgCallback getMsgClk;
    getMqttSubscribeMsgCallback getSubscrbClk;
    bool isConnected;
    bool forceQuit;
    bool ifGetMqttServerInfo;
    bool ifGetHotelRoomInfo;
    MQTTClient client;
    MqttServerInfo mqttServerInfo;
    HotelRoomInfo hotelRoomInfo;
    DeviceInfo deviceInfo;
    std::shared_ptr<std::thread> _thread;
    std::mutex mtx;
    std::mutex mtx_info;
    std::condition_variable cv;
};


#endif // !_HF_MQTT_MANAGER_H_
