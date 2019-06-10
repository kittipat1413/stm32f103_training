#ifndef UC15MQTT_H_
#define UC15MQTT_H_

#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "UC15Queue.h"
#include "UC15Uart.h"
// #include "UC15TCP.h"

#define MQTT_VERSION_3_1      3
#define MQTT_VERSION_3_1_1    4

// MQTT_VERSION : Pick the version
//#define MQTT_VERSION MQTT_VERSION_3_1
#ifndef MQTT_VERSION
#define MQTT_VERSION MQTT_VERSION_3_1_1
#endif

// MQTT_MAX_PACKET_SIZE : Maximum packet size
//#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 100
//#endif

// MQTT_KEEPALIVE : keepAlive interval in Seconds
#ifndef MQTT_KEEPALIVE
#define MQTT_KEEPALIVE 60
#endif

// MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds
#ifndef MQTT_SOCKET_TIMEOUT
#define MQTT_SOCKET_TIMEOUT 15
#endif

// MQTT_MAX_TRANSFER_SIZE : limit how much data is passed to the network client
//  in each write call. Needed for the Arduino Wifi Shield. Leave undefined to
//  pass the entire MQTT packet in each write call.
//#define MQTT_MAX_TRANSFER_SIZE 80

// Possible values for client.state()
#define MQTT_CONNECTION_TIMEOUT     -4
#define MQTT_CONNECTION_LOST        -3
#define MQTT_CONNECT_FAILED         -2
#define MQTT_DISCONNECTED           -1
#define MQTT_CONNECTED               0
#define MQTT_CONNECT_BAD_PROTOCOL    1
#define MQTT_CONNECT_BAD_CLIENT_ID   2
#define MQTT_CONNECT_UNAVAILABLE     3
#define MQTT_CONNECT_BAD_CREDENTIALS 4
#define MQTT_CONNECT_UNAUTHORIZED    5

#define MQTTCONNECT     1 << 4  // Client request to connect to Server
#define MQTTCONNACK     2 << 4  // Connect Acknowledgment
#define MQTTPUBLISH     3 << 4  // Publish message
#define MQTTPUBACK      4 << 4  // Publish Acknowledgment
#define MQTTPUBREC      5 << 4  // Publish Received (assured delivery part 1)
#define MQTTPUBREL      6 << 4  // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP     7 << 4  // Publish Complete (assured delivery part 3)
#define MQTTSUBSCRIBE   8 << 4  // Client Subscribe request
#define MQTTSUBACK      9 << 4  // Subscribe Acknowledgment
#define MQTTUNSUBSCRIBE 10 << 4 // Client Unsubscribe request
#define MQTTUNSUBACK    11 << 4 // Unsubscribe Acknowledgment
#define MQTTPINGREQ     12 << 4 // PING Request
#define MQTTPINGRESP    13 << 4 // PING Response
#define MQTTDISCONNECT  14 << 4 // Client is Disconnecting
#define MQTTReserved    15 << 4 // Reserved

#define MQTTQOS0        (0 << 1)
#define MQTTQOS1        (1 << 1)
#define MQTTQOS2        (2 << 1)

typedef void (*callback)(char* topic ,char* playload,unsigned char length);

typedef struct UC15Mqtt {
	UC15Uart* uc;
	uint16_t nextMsgId;
	bool pingOutstanding;
	bool connected;
} UC15Mqtt;

bool UC15MqttInit(UC15Mqtt* mq, UC15Uart* uc);
bool UC15MqttConnectMQTTServer(UC15Mqtt* mq, char* host, char* port);
bool UC15MqttDisconnectMQTTServer(UC15Mqtt* mq);

bool UC15MqttSend(UC15Mqtt* mq, uint8_t* pkg, uint16_t pkglen);
uint16_t UC15MqttReceive(UC15Mqtt* mq, uint8_t* dst);

// Packatizer
typedef struct MqttPack{
	uint8_t p[MQTT_MAX_PACKET_SIZE];
	uint16_t len;

	char topic[MQTT_MAX_PACKET_SIZE];
	uint8_t payload[MQTT_MAX_PACKET_SIZE];
	uint16_t payloadlength;

	char header[30];
} MqttPack;

// client to server
MqttPack MqttPackCONNECT(char* clientID, char* username, char* password, char* willTopic, uint8_t willQos, bool willRetain, char* willMessage);
MqttPack MqttPackPUBLISH(char* topic, uint8_t* payload, uint16_t payloadlength, bool retained);
MqttPack MqttPackSUBSCRIBE(char* topic);
MqttPack MqttPackUNSUBSCRIBE(char* topic);
MqttPack MqttPackPINGREQ(void);
MqttPack MqttPackDISCONNECT(void);

MqttPack MqttUnpack(uint8_t* src, uint16_t* len);
bool MqttUnpackPUBLISH(MqttPack* pack);

// server to client
uint8_t* MqttPackSUBACK();
uint8_t* MqttPackUNSUBACK();
uint8_t* MqttPackPINGRESP();

// both
uint8_t MqttPackPUBACK();
uint8_t MqttPackPUBREC();
uint8_t MqttPackPUBREL();
uint8_t MqttPackPUBCOMP();

#endif