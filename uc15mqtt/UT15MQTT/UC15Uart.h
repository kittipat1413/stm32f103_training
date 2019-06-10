#ifndef UC15UART_H_
#define UC15UART_H_

#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "UC15Queue.h"

#define BUFFER_SIZE 300

#define CR      '\r'
#define LF      '\n'
#define VALIDMESSAGE_END_OK 1
#define VALIDMESSAGE_END_ERROR 2
#define VALIDMESSAGE_URC 3
#define READY_TO_SEND 4

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

typedef struct UC15Uart {
	//usart interface
	USART_TypeDef* USART;
	
	//queue and internal buffer
	UC15Queue* InboundQueue;
	char buffer[BUFFER_SIZE];

	// char* topic;
	// bool external_topic;
	bool QIURC_flag;
} UC15Uart;

//initialize
void UC15UartInit(UC15Uart* uc, USART_TypeDef* usartx, UC15Queue* q);

//send single byte
void UC15Uart_send_byte(uint8_t b, USART_TypeDef* usartx);

//send string
void UC15UartSend(UC15Uart* uc, char* s);
void UC15UartSendDEBUG(UC15Uart* uc, char* s);

//check incoming data
bool UC15UartIsAvailable(UC15Uart* uc);

//read single byte
uint8_t UC15UartReadByte(UC15Uart* uc);

//read message <CR><LF>...<CR><LF>
int8_t UC15UartRead(UC15Uart* uc);

//get string
bool UC15UartGetString(UC15Uart* uc, char* dst);

//flush

//check uart ready
bool UC15UartIsReady(UC15Uart* uc);

// Get
// bool UC15UartGetRevision(UC15Uart* uc, char* dst);
char* UC15UartGetRevision(UC15Uart* uc);

// bool UC15UartGetIMI(UC15Uart* uc, char* dst);
char* UC15UartGetIMI(UC15Uart* uc);

bool UC15UartGetSerialNumber(UC15Uart* uc, char* dst);
bool UC15UartGetManufacturerIdentification(UC15Uart* uc, char* dst);
bool UC15UartGetCurrentConfig(UC15Uart* uc, char* dst);
bool UC15UartGetSignalStrength(UC15Uart* uc, char* dst);



// int8_t UC15UartGetResponse(UC15Uart* uc, uint32_t timeout);


// // reset
// bool UC15UartReset(UC15Uart* uc);

// // ready
// int8_t UC15UartWaitString(UC15Uart* uc, char* dst, char* str, uint32_t timeout);
// int8_t UC15UartWait(UC15Uart* uc, char* dst, char* str, uint32_t timeout);
// bool UC15UartWaitReady(UC15Uart* uc, char* dst);
// int8_t UC15UartWaitOK(UC15Uart* uc, char* dst, uint32_t timeout);

// //internet
bool UC15UartInternetConfig(UC15Uart* uc, char* dst, char* apn, char* user, char* password);
bool UC15UartInternetConnect(UC15Uart* uc);
bool UC15UartInternetDisconnect(UC15Uart* uc);
char* UC15UartInternetGetIP(UC15Uart* uc);

bool UC15UartTCPOpen(UC15Uart* uc, unsigned char contexid, unsigned char connectid, char* service_type, char* ip_url, char* remote_port, char* local_port, unsigned char access_mode);
bool UC15UartTCPState(UC15Uart* uc, unsigned char connectid);
bool UC15UartTCPClose(UC15Uart* uc, unsigned char connectid);

bool TCPStartSend(UC15Uart* uc, unsigned char connectid);
bool TCPSend(UC15Uart* uc,uint8_t* data, uint16_t length);
bool TCPStopSend(UC15Uart* uc);

bool UC15UartTCPHaveComingByte(UC15Uart* uc);
uint16_t UC15UartTCPRead(UC15Uart* uc, unsigned char connectid, char* dst);
#endif