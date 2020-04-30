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



/**
 * @brief mqtt管理类
 */
class hfMqttManager
{
public:
    typedef std::shared_ptr<hfMqttManager> Ptr;
    hfMqttManager();
    ~hfMqttManager();

    /**
     * @brief 初始化函数
     * @param deviceCode   设备id
     * @param httpMqttServerUrl  鉴权url
     * @param httpRoomInfoUrl   获取房间信息url
     * @param getMsgClk     获取房间信息回调
     * @param getSubscrbClk 收到mqtt订阅消息回调
     * @return bool
     */
    bool hfMqttManagerInit(const std::string & deviceCode,
                           const std::string &httpMqttServerUrl, 
                           const std::string &httpRoomInfoUrl, 
                           getHotelRoomMsgCallback getMsgClk, 
                           getMqttSubscribeMsgCallback getSubscrbClk);


    /**
     * @brief 发布一条消息
     * @param topic  发布地址
     * @param msg    发布消息（json）
     * @param qos    消息等级（默认2）
     * @param retained 是否持久（默认不）
     * @return bool 发布是否成功
     */
    bool hfMqttClientPublishMsg(const std::string& topic, const std::string& msg, int qos = 2, int retained = 0);

    /**
     * @brief 订阅消息
     * @param topic  订阅地址
     * @param qos    消息等级（默认2）
     * @return bool 订阅是否成功
     */
    bool hfMqttClientSubscribe(const std::string& topic, int qos = 2);

    /**
     * @brief 订阅多条消息
     * @param topic topic:qos键值对 
     * @return bool 订阅是否成功
     */
    bool hfMqttClientSubscribeMany(const std::map<std::string, int> & topics);

     /**
     * @brief 取消订阅消息
     * @param topic 主题
     * @return bool 取消订阅是否成功
     */   
    bool hfMqttClientUnsubscribe(const  std::string &topic);

     /**
     * @brief 取消订阅多条消息
     * @param topic 主题
     * @return bool 取消订阅是否成功
     */   
    bool hfMqttClientUnsubscribeMany(const std::vector<std::string> & topics);


    /**
     * @brief 主动获取鉴权接口
     * @param mqttInfo 输出鉴权信息
     * @return bool 是否成功
     */
    bool getDeviceAuthorityInfo(MqttServerInfo & mqttInfo);  

     /**
     * @brief 主动获取房间信息接口
     * @param roomInfo 输出获取房间信息
     * @return bool 是否成功
     */      
    bool getHotelRoomInfo(HotelRoomInfo & roomInfo);   

    /**
     * @brief 上报通行记录接口
     * @param alarmType 消息类型
     * @param attrList 设备状态信息
     * @param passRecordList 通行记录信息
     * @return bool 是否成功
     */ 
    bool hfMqttPublishPassRecords(AlarmTypeCode alarmType, const std::vector<AttributesEntity>& attrList, const std::vector<PassRecord>& passRecordList); 
    

private:
   void hfMqttClientUninit();
   bool hfMqttClientInit_(const MqttServerInfo& serverInfo);
   bool hfMqttClientReSubscribe();

   bool getMqttServerMsgByDeviceCode(const std::string &httpUrl, const std::string & deviceCode, MqttServerInfo & serverInfo);

   bool getRoomInfoByDeviceCode(const std::string &httpUrl, const std::string & deviceCode, HotelRoomInfo & roomInfo);
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
