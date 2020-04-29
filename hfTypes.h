#ifndef _HF_TYPES_H_
#define _HF_TYPES_H_

#include <string>
#include <vector>
#include <map>
#include <memory>


#define  HF_CODE "huafu"    // brand
enum ServerReturnCode
{
    CODE_SUCCESS = 0,
    CODE_COMMEN_ERR = 1,
    CODE_LOGIN_EXIT = 10,
    CODE_INCORRET_PARAM = 11,
    CODE_DATA_NO_EXIST = 12,
    CODE_CHECK_FAILURE = 13,
    CODE_DATA_EXIST = 15,
    CODE_USER_NOT_EXIST = 16
};

enum AlarmTypeCode
{
  EVENT_BODY_DETECT = 10000, //人体感应事件
  EVENT_EMERGENCY_CONTROL = 10001, //紧急遥控按钮事件
  EVETN_EMERGENCY_BUTTON_ALARM = 10009, //紧急按钮告警
  EVENT_BODY_DETECT_ALARM = 10010, //人体感应告警
  EVENT_OCCLUSION_ALARM = 10011, //遮挡告警
  EVENT_VIDEO_LOSS_ALARM = 10012, //视频丢失
  EVENT_FACE_DETECT = 10015, // 人脸检测事件
  EVENT_DOORBELL_ALARM = 10016, //智能门铃告警
};


struct MqttServerInfo
{
    std::string clientId;
    std::string serverAddr;
    unsigned int port;
    std::string password;
};

struct HotelRoomInfo
{
    std::string floor; //楼层
    std::string hotelId; //酒店id 
    std::string hotelName;  //酒店名称
    std::string hotelEnglishName; //酒店英⽂文名称
    std::string hotelIcon;   //酒店icon string
    std::string houseTypeId;  //房间类型Id string
    std::string houseTypeName; // 房间类型名称string
    std::string roomImgUrl;  //图⽚片链接string
    std::string phone;  //电话分机string
    std::string remark;  //备注string
    std::string roomId;  //房间id string
    std::string roomNum; //房号string
};

struct DeviceInfo
{
    std::string gwCode; //网关编号
    std::string entityCode; //设备code
    std::string typeCode;  //设备类型编号
};

struct AlarmInfo
{
    AlarmTypeCode alarmCode;
    std::string msg;  //非必填
};

struct AttributesEntity
{
    std::string attributeCode;  //battery:电量 ; 状态
    std::string value;
};

//通行记录
struct PassRecord
{
    std::string id; //人员编号 陌生人填-1
    std::string name; //识别通行人员姓名,陌生人传name: 陌生人
    std::string passPhoto; //识别人脸抓拍照片base64字符串上报 、陌生人必填字段
    std::string passMode; //认证模式:FACE（人脸）、IC卡（门禁卡）、CONTROL（远程控制开门）
    int resultType;      // int 0 ：通过 1 ：不通过 代表該人員被禁）
    std::string passTime;  // String  认证时间2019-12-26 12:31:30 、 陌生人必填字段
    int personType;         //int 人员类型
    std::string code;     //非必填
    std::string reservcer2;  //非必填，预留字段
    std::string reservcer3;  //非必填，预留字段
};

struct MqttUploadMsg
{
    std::string reqId; //请求id，32位随机字符串
    std::string msgType; //UP同步 通行记录，设备状态上报，告警消息 、MSG_ACK 人脸接收确认
    DeviceInfo deviceInfo;
    std::string timestamp;
    AlarmInfo alarmInfo;
    std::vector<AttributesEntity> attrList;
    std::vector<PassRecord> passRecordList;
};


#endif // !_HF_TYPES_H_
