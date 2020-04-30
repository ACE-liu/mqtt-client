#include <iostream>
#include <string>
#include "MQTTClient.h"
#include <stdlib.h>
#include <string.h>
#include "hfMqttManager.h"
#include <vector>
#include <map>


using namespace std;



void  getRoomInfoCallback(const HotelRoomInfo& roomInfo)
{
    std::cout<<"floor: "<<roomInfo.floor<<endl;
    std::cout<<"hotelEnglishName: "<<roomInfo.hotelEnglishName<<endl;
    std::cout<<"hotelIcon: "<<roomInfo.hotelIcon<<endl;
    std::cout<<"hotelId: "<<roomInfo.hotelId<<endl;
    std::cout<<"hotelName: "<<roomInfo.hotelName<<endl;
    std::cout<<"houseTypeId: "<<roomInfo.houseTypeId<<endl;
    std::cout<<"houseTypeName: "<<roomInfo.houseTypeName<<endl;
    std::cout<<"phone: "<<roomInfo.phone<<endl;
    std::cout<<"remark: "<<roomInfo.remark<<endl;
    std::cout<<"roomId: "<<roomInfo.roomId<<endl;
    std::cout<<"roomImgUrl: "<<roomInfo.roomImgUrl<<endl;
    std::cout<<"roomNum: "<<roomInfo.roomNum<<endl;
} 

void getSubsribeMsgCallback(const std::string topic, const std::string msg)
{
    std::cout<<"topic: "<<topic<<endl;
    std::cout<<"msg: \n"<<msg<<endl;
}

int main(int argc, char *argv[])
{
    string address = "172.16.19.150";
    unsigned int port = 1883;
    string clientId = "liuliuclient001";
    string userName = "admin";
    string password = "public";
    string topic = "topic:hf/001";
    string msg = "hello world.";
    int qos = 2;
    int rc;
    int retained =1;
    char*mtopic[3] = {"topic:shimao/face/#", "topic:shimao/table/#", "topic:shimao/leg/#"};
    int mQos[3] = {2,2,2};
    hfMqttManager mqttManager;
    string deviceCode = "123456";
    string httpMqttUrl = "http://127.0.0.1";
    string httpRoomMsgUrl = "http://127.0.0.1";
    
    // if(mqttManager.hfMqttClientInit(address.c_str(), port, clientId.c_str(), userName.c_str(), password.c_str(), msgarrvd) == false)
    // {
    //     cout<<"connect failed to mqtt server..."<<std::endl;
    //     return -1;
    // }
    if(mqttManager.hfMqttManagerInit(deviceCode, httpMqttUrl, httpRoomMsgUrl, getRoomInfoCallback, getSubsribeMsgCallback) == false)
    {
        cout<<"connect failed to mqtt server..."<<std::endl;
        return -1;
    }
    cout<<"connect success to mqtt server..."<<std::endl;
   
    // if(hfMqttClientSubscribe(topic.c_str(), qos) == false)
    // {
    //     cout <<"subscribe topic: "<<topic<<" failed...."<<std::endl;
    //     return -1;
    // }
    map<string, int> topics;
    topics["topic:shimao/face/#"] = 2;
    topics["topic:shimao/table/#"] = 2;
    topics["topic:shimao/leg/#"] = 2;
    if(mqttManager.hfMqttClientSubscribeMany(topics) == false)
    {
        cout << "subscribe topic: "<<topic<<" failed...."<<std::endl;
        return -1;
    }

    int ch;
    do
    {
        ch = getchar();
        printf("get char : %c \n", ch);
        if(ch == 'p')
        {
            printf("start send public message.\n");
            if (mqttManager.hfMqttClientPublishMsg(topic, msg, qos, retained) == false)
                cout <<"publish message failed....\n"<<std::endl;

        }
    } while (ch!='Q' && ch != 'q');

    return 0;
}