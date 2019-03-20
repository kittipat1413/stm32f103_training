#ifndef NBUART_H_
#define NBUART_H_

#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "NBQueue.h"

#define BUFFER_SIZE 300

#define UNSOLICITED 1
#define PAYLOAD 2
#define ENDTAG 3

#define READ_TIMEOUT      -1
#define READ_OVERFLOW     -2
#define READ_INCOMPLETE   -3
#define READ_COMPLETE_OK    2
#define READ_COMPLETE_ERROR 3
#define READ_COMPLETE_NEUL 4
#define READ_COMPLETE_NSOMMI 5
#define READ_COMPLETE_REBOOT 6
#define STOPPERLEN        12

#define END_LINE        "\x0D\x0A"
#define END_OK          "\x0D\x0A\x4F\x4B\x0D\x0A"
#define NOMSG           "\x23"
#define END_ERROR       "\x0D\x0A\x45\x52\x52\x4F\x52\x0D\x0A"
#define NEUL "\x4E\x65\x75\x6C\x20"

typedef struct NBUart {
	USART_TypeDef* USART;
	NBQueue* InboundQueue;
	char buffer[BUFFER_SIZE];

	bool Reboot;
} NBUart;

void NBUartInit(NBUart* nb, USART_TypeDef* usartx, NBQueue* q);
void NBUartSend(NBUart* nb, char* s);
void NBUartSendDEBUG(NBUart* nb, char* s);
bool NBUartIsAvailable(NBUart* nb);
uint8_t NBUartReadByte(NBUart* nb);
int8_t NBUartGetResponse(NBUart* nb, uint32_t timeout);
bool NBUartGetString(NBUart* nb, char* dst);

typedef struct Response {
	char* Payload;
	bool Status;
} Response;

typedef struct ResponseMessage {
	char* Message;
	bool EndingStatus;
} ResponseMessage;

ResponseMessage NBUartParse(NBUart* nb, char *rawstr, char* prefix);

// reset
bool NBUartReset(NBUart* nb);

// Get NB-iot parameter test
bool NBUartGetIMI(NBUart* nb, char* dst);
bool NBUartGetManufacturerModel(NBUart* nb, char* dst);
bool NBUartGetManufacturerRevision(NBUart* nb, char* dst);
bool NBUartGetIPAddress(NBUart* nb, char* dst);
int8_t NBUartGetSignalStrength(NBUart* nb);

// Network command
typedef struct Socket {
	uint8_t ID;
	uint8_t Status;
	uint16_t Port;
	uint16_t Msglen;
	uint16_t OutboundMsglen;
	uint16_t InboundMsglen;
} Socket;

// Network command test
bool NBUartAttachNetwork(NBUart* nb);
Socket NBUartCreateSocket(NBUart* nb, uint16_t port);
bool NBUartCloseSocket(NBUart* nb, Socket* s);

// Packet handler
bool NBUartSendPacket(NBUart* nb, Socket* s, char* destIP, uint16_t destPort, uint8_t* payload, uint16_t size);
bool NBUartFetchPacket(NBUart* nb, Socket* socket, uint16_t len, char* dst);

// UDP handler
#define SOCKET_SIZE 7
#define UDP_BUFFER_SIZE 1024
#define UART_CHUNK_SIZE 7
typedef struct UDPConnection {
	NBUart* NB;
	Socket Sock;
	char DestinationIP[20];
	uint16_t DestinationPort;
	uint8_t InboundBuffer[UDP_BUFFER_SIZE];
	uint16_t InboundBufferlen;
	uint8_t OutboundBuffer[UDP_BUFFER_SIZE];
	uint16_t OutboundBufferlen;
} UDPConnection;

char* UDPGetIP(UDPConnection* udp);
uint16_t UDPGetPort(UDPConnection* udp);
uint16_t UDPAvailable(UDPConnection* udp);

bool UDPInit(UDPConnection* udp, NBUart* nb);
bool UDPConnect(UDPConnection* udp, char* destIP, uint16_t destPort);
bool UDPDisconnect(UDPConnection* udp);
bool UDPSetIP(UDPConnection* udp, char* ip);
bool UDPSetPort(UDPConnection* udp, uint16_t port);
uint16_t UDPWrite(UDPConnection* udp, const uint8_t* buffer, uint16_t size);
bool UDPSendPacket(UDPConnection* udp);
int UDPParsePacket(UDPConnection* udp);
int UDPRead(UDPConnection* udp, uint8_t* buffer, uint16_t len);
void UDPFlush(UDPConnection* udp);
void UDPFlushInbound(UDPConnection* udp);
void UDPFlushOutbound(UDPConnection* udp);
bool UDPIsReboot(UDPConnection* udp);
#endif