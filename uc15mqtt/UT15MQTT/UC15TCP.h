#ifndef TCP_h
#define TCP_h

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

typedef struct TCP {
	UC15Uart* uc;
	unsigned char ReceiveConnectID;
} TCP;

//done
bool UC15TCPInit(TCP* tcp, UC15Uart* uc);
bool UC15TCPOpen(TCP* tcp, unsigned char contexid, unsigned char connectid, char* service_type, char* ip_url, char* remote_port, char* local_port, unsigned char access_mode);

//testing
bool UC15TCPState(TCP* tcp, unsigned char connectid);
bool UC15TCPSend(TCP* tcp, char* data);
bool UC15TCPStartSend(TCP* tcp, unsigned char contexid);
bool UC15TCPStopSend(TCP* tcp);
int UC15TCPReadBuffer(TCP* tcp, unsigned char connectid);
bool UC15TCPReceiveAvailable(TCP* tcp);
bool UC15TCPCloseWithContex(TCP* tcp, unsigned char contexid);
bool UC15TCPClose(TCP* tcp);

//pending
// bool UC15TCPOpenSimple(TCP* tcp, char* ip_url, char* port);
// bool UC15TCPStartSendWithLength(TCP* tcp, unsigned char contexid, int len);
// bool UC15TCPStartSendNull(TCP* tcp);
// bool UC15TCPWaitSendFinish(TCP* tcp);
// int UC15TCPReadBufferWithContex(TCP* tcp, unsigned char contexid, int max_len);
// int UC15TCPReadBufferWithLength(TCP* tcp, int max_len);
// void UC15TCPPing(TCP* tcp, unsigned char contexid, char* ip_url);
// void UC15TCPwrite(TCP* tcp, char data);
// void UC15TCPprint(TCP* tcp, char* data);
// void UC15TCPprintln(TCP* tcp, char* data);
// void UC15TCPprintInt(TCP* tcp, int data);
// void UC15TCPprintlnInt(TCP* tcp, int data);
// bool UC15TCPCloseWithContex(TCP* tcp, unsigned char contexid);
// bool UC15TCPClose(TCP* tcp);
// bool UC15TCPCheckConnectionWithContex(TCP* tcp, unsigned char query_type, unsigned char contexid);
// bool UC15TCPCheckConnection(TCP* tcp);
// bool UC15TCPNTP(TCP* tcp, unsigned char contexid, char* ip_url, char* port, char* dst);
#endif