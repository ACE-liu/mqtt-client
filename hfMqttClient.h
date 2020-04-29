#include "MQTTClient.h"
#include <string>



bool hfMqttClientInit(const char * serverAddr, unsigned int port, const char * clientId, 
                      const char * userName, const char * password,MQTTClient_messageArrived* msgArdCallback);


bool hfMqttClientPublishMsg(const char * topic, const char * msg, int qos = 2, int retained = 0);


bool hfMqttClientSubscribe(const char * topic, int qos = 2);

bool hfMqttClientSubscribeMany(char * const* topic, int topicCount, int * qos =NULL);

bool hfMqttClientUnsubscribe(const char * topic);

bool hfMqttClientUnsubscribeMany(char * const* topic, int topicCount);



void hfMqttClientUninit();



