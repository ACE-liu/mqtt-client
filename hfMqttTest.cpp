
#include <iostream>
#include <string>
#include "MQTTClient.h"
#include <stdlib.h>
#include <string.h>

using namespace std;


#define ADDRESS     "tcp://172.16.19.150:1883"
#define CLIENTID    "ClientSubliuliu"
#define TOPIC       "topic:hf/001"
#define PAYLOAD     "{ \
\"retcode\": 0, \
\"msg\": \"success\", \
\"data\": { \
\"clientId\": \"1578480354203-s1dpum-ln\",\
\"clientId1\": \"nl-mupd1s-3024530848751\",\
\"serviceAddr\": \"47.103.67.177\",\
\"servicePort\": \"1883\",\
\"password\": \"0dxktpsf9wqt5bmzq8t9liuliu\"\
} \
}"

#define QOS         2
#define TIMEOUT     10000L
#define userName "admin"
#define pwd "public"
volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n", message->payloadlen, (char*)message->payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char * argv[])
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    int mcount = 3;
    char*mtopic[3] = {"topic:shimao/face/#", "topic:shimao/table/#", "topic:shimao/leg/#"};
    int mQos[3] = {2,2,2};
    char * sendPaload = "1+1+1+1?????????== hello world.";

    if ((rc = MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to create client, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto exit;
    }

    if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to set callbacks, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = userName;
    conn_opts.password = pwd;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }

    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);

    // if ((rc = MQTTClient_subscribe(client, TOPIC, QOS)) != MQTTCLIENT_SUCCESS)
    // {
    // 	printf("Failed to subscribe, return code %d\n", rc);
    // 	rc = EXIT_FAILURE;
    // }
    if ((rc = MQTTClient_subscribeMany(client, mcount, mtopic, mQos)) != MQTTCLIENT_SUCCESS)
    {
    	printf("Failed to subscribe, return code %d\n", rc);
    	rc = EXIT_FAILURE;
    }
    else
    {
    	int ch;
    	do
    	{
        	ch = getchar();
            printf("get char : %c \n", ch);
            if(ch == 'p')
            {
                printf("start send public message.\n");
                if ((rc = MQTTClient_publish(client, TOPIC, (int)strlen(sendPaload), sendPaload, QOS, 1, &token)) != MQTTCLIENT_SUCCESS)
                {
                    printf("Failed to publish message, return code %d\n", rc);
                }
                else 
                {
                    printf("Success to publish message, return code %d\n", rc);
                }
                printf("end send publish message.\n");

            }
            else if(ch == 'c')
            {
                pubmsg.payload = NULL;
                pubmsg.payloadlen = 0;
                pubmsg.qos = QOS;
                pubmsg.retained = 0;
                printf("start send public message.\n");
                if ((rc = MQTTClient_publishMessage(client, TOPIC, &pubmsg, NULL)) != MQTTCLIENT_SUCCESS)
                {
                    printf("Failed to publish message, return code %d\n", rc);
                }
                else 
                {
                    printf("Success to publish message, return code %d\n", rc);
                }
                printf("end send publish message.\n");               
            }
    	} while (ch!='Q' && ch != 'q');

        // if ((rc = MQTTClient_unsubscribe(client, TOPIC)) != MQTTCLIENT_SUCCESS)
        // {
        // 	printf("Failed to unsubscribe, return code %d\n", rc);
        // 	rc = EXIT_FAILURE;
        // }
        if ((rc = MQTTClient_unsubscribeMany(client,mcount, mtopic)) != MQTTCLIENT_SUCCESS)
        {
        	printf("Failed to unsubscribe, return code %d\n", rc);
        	rc = EXIT_FAILURE;
        } 
    }

    if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
    {
    	printf("Failed to disconnect, return code %d\n", rc);
    	rc = EXIT_FAILURE;
    }
destroy_exit:
    MQTTClient_destroy(&client);
exit:
    return rc;
}