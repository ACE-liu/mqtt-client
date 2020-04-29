#include "hfMqttClient.h"
#include "iostream"
#include <string.h>
#include <unistd.h>

using namespace std;

struct conInfo{
    string conUrl;
    string clientId;
    string userName;
    string password;
    MQTTClient_messageArrived* msgArdCallback;
};
static MQTTClient client =NULL;
static bool isConnected = false;
static conInfo _conInfo;
static bool hfMqttClientInit_(const conInfo& connectInfo);

static void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
    isConnected = false;
    while(!isConnected)
    {
         if(hfMqttClientInit_(_conInfo) == false)
         {
             sleep(2);
         }
    }
    printf("connect success.\n");
}

static void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
}

static void updateConInfo(const char * serverAddr, unsigned int port,const char * clientId, 
                          const char * userName, const char * password, MQTTClient_messageArrived* msgArdCallback)
{
    _conInfo.conUrl = string(serverAddr) + ":" + to_string(port);
    _conInfo.clientId = clientId;
    _conInfo.userName = userName;
    _conInfo.password = password;
    _conInfo.msgArdCallback = msgArdCallback;
}

static bool hfMqttClientInit_(const conInfo& connectInfo)
{
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    if ((rc = MQTTClient_create(&client, _conInfo.conUrl.c_str(), _conInfo.clientId.c_str(),
        MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to create client, return code %d\n", rc);
        isConnected = false;
        return false;
    }

    if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, _conInfo.msgArdCallback, delivered)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to set callbacks, return code %d\n", rc);
        isConnected = false;
        return false;
    }
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = _conInfo.userName.c_str();
    conn_opts.password = _conInfo.password.c_str();
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

bool hfMqttClientInit(const char * serverAddr, unsigned int port,const char * clientId, 
                      const char * userName, const char * password, MQTTClient_messageArrived* msgArdCallback)
{
    updateConInfo(serverAddr, port, clientId, userName, password, msgArdCallback);
    return hfMqttClientInit_(_conInfo);
}


bool hfMqttClientPublishMsg(const char * topic, const char * msg, int qos, int retained)
{
    int rc;
    if(topic==NULL||msg==NULL)
        return false;

    if ((rc = MQTTClient_publish(client, topic, (int)strlen(msg), msg, qos, retained, NULL)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to publish message, return code %d\n", rc);
        return false;
    }

    printf("Success to publish message, return code %d\n", rc);
    return true;
}

bool hfMqttClientSubscribe(const char * topic, int qos)
{
    int rc;
    if ((rc = MQTTClient_subscribe(client, topic, qos)) != MQTTCLIENT_SUCCESS)
    {
    	printf("Failed to subscribe, return code %d\n", rc);
        return false;
    }
    return true;
}


bool hfMqttClientSubscribeMany(char * const* topic, int topicCount, int * qos)
{
    int rc;
    bool needDelete = false;
    if(topic==NULL||topicCount == 0)
        return false;
    if(qos==NULL)
    {
        qos = new int[topicCount];
        for(int i = 0; i <topicCount; i++)
            qos[i] = 2;
        needDelete =true;
    }
    if ((rc = MQTTClient_subscribeMany(client, topicCount, topic, qos)) != MQTTCLIENT_SUCCESS)
    {
    	printf("Failed to subscribe, return code %d\n", rc);
        if(needDelete)
            delete[] qos;
    	return false;
    }
    if(needDelete)
       delete[] qos;
    return true;
}


bool hfMqttClientUnsubscribe(const char * topic)
{
    int rc;
    if ((rc = MQTTClient_unsubscribe(client, topic)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to unsubscribe, return code %d\n", rc);
        return false;
    }   
    return true;
}

bool hfMqttClientUnsubscribeMany(char * const* topic, int topicCount)
{
    int rc;
    if ((rc = MQTTClient_unsubscribeMany(client,topicCount, topic)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to unsubscribe, return code %d\n", rc);
        return false;
    }   
    return true;
}

void hfMqttClientUninit()
{
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);    
    client = NULL;
    isConnected = false;
}

