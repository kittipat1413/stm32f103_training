/*


This software is released under the MIT License.
*/

#ifndef BC95_H_
#define BC95_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
}
#endif

#define MAXSOCKET 7
#define END_LINE        "\x0D\x0A"
#define END_OK          "\x0D\x0A\x0D\x0A\x4F\x4B\x0D\x0A"
#define END_ERROR       "\x0D\x0A\x0D\x0A\x45\x52\x52\x4F\x52\x0D\x0A"
#define STOPPERLEN        12

////////////////////////////////////////////////////////
// settings
////////////////////////////////////////////////////////

#define DATA_BUFFER_SIZE                400

#define BC95_USE_EXTERNAL_BUFFER        1
#define BC95_PRINT_DEBUG                0
#define BC95_DEFAULT_SERIAL_TIMEOUT     500
#define BC95_BUFFER_SIZE                DATA_BUFFER_SIZE

#define BC95UDP_USE_EXTERNAL_BUFFER     1
#define BC95UDP_SHARE_GLOBAL_BUFFER     1
#define BC95UDP_SERIAL_READ_CHUNK_SIZE  7
#define BC95UDP_BUFFER_SIZE             DATA_BUFFER_SIZE

#define NTP_DEFAULT_SERVER              "time.nist.gov"

#define COAP_ENABLE_ACK_CALLBACK        1
#define COAP_CONFIRMABLE                0

typedef struct SOCKD {
    uint8_t sockid;
    uint8_t status;
    uint16_t port;
    uint16_t msglen;
    uint16_t buff_msglen;
    uint16_t bc95_msglen;
} SOCKD;
// struct SOCKD ;


struct BC95_str {
	USART_TypeDef * USART;
	SOCKD socketpool[MAXSOCKET];

};
typedef struct BC95_str BC95_str;

char* getSerialResponse(BC95_str* bc95, char *prefix);
void BC95_usart_puts(char* s, USART_TypeDef* usartx);
void BC95_reset(BC95_str* bc95);
void BC95_init(USART_TypeDef* usartx, BC95_str* bc95);
char* BC95_fetchSocketPacket(BC95_str* bc95, SOCKD* socket, uint16_t len);
bool BC95_attachNetwork(BC95_str* bc95);
char* BC95_getIMI(BC95_str* bc95);
char* BC95_getManufacturerModel(BC95_str* bc95);
char* BC95_getManufacturerRevision(BC95_str* bc95);
char* BC95_getIPAddress(BC95_str* bc95);
int8_t BC95_getSignalStrength(BC95_str* bc95);
SOCKD* BC95_createSocket(BC95_str* bc95, uint16_t port);
char* BC95_sendPacket(BC95_str* bc95, SOCKD* socket, char* destIP, uint16_t destPort, uint8_t* payload, uint16_t size);
char* BC95_closeSocket(BC95_str* bc95, SOCKD* socket);
void BC95_push(BC95_str* bc95,char b);

#endif