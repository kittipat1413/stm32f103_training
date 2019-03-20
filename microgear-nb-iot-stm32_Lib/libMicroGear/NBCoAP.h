#ifndef NBCoAP_H_
#define NBCoAP_H_

#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "NBQueue.h"
#include "NBUart.h"
#include "NBDNS.h"

#define MAX_CALLBACK 10

#define COAP_HEADER_SIZE 4
#define COAP_OPTION_HEADER_SIZE 1
#define COAP_PAYLOAD_MARKER 0xFF
#define MAX_OPTION_NUM 10
#define BUF_MAX_SIZE 160
#define COAP_DEFAULT_PORT 5683

#define RESPONSE_CODE(class, detail) ((class << 5) | (detail))
#define COAP_OPTION_DELTA(v, n) (v < 13 ? (*n = (0xFF & v)) : (v <= 0xFF + 13 ? (*n = 13) : (*n = 14)))

typedef enum {
    COAP_CON = 0,
    COAP_NONCON = 1,
    COAP_ACK = 2,
    COAP_RESET = 3
} COAP_TYPE;

typedef enum {
    COAP_GET = 1,
    COAP_POST = 2,
    COAP_PUT = 3,
    COAP_DELETE = 4
} COAP_METHOD;

typedef enum {
    COAP_CREATED = RESPONSE_CODE(2, 1),
    COAP_DELETED = RESPONSE_CODE(2, 2),
    COAP_VALID = RESPONSE_CODE(2, 3),
    COAP_CHANGED = RESPONSE_CODE(2, 4),
    COAP_CONTENT = RESPONSE_CODE(2, 5),
    COAP_BAD_REQUEST = RESPONSE_CODE(4, 0),
    COAP_UNAUTHORIZED = RESPONSE_CODE(4, 1),
    COAP_BAD_OPTION = RESPONSE_CODE(4, 2),
    COAP_FORBIDDEN = RESPONSE_CODE(4, 3),
    COAP_NOT_FOUNT = RESPONSE_CODE(4, 4),
    COAP_METHOD_NOT_ALLOWD = RESPONSE_CODE(4, 5),
    COAP_NOT_ACCEPTABLE = RESPONSE_CODE(4, 6),
    COAP_PRECONDITION_FAILED = RESPONSE_CODE(4, 12),
    COAP_REQUEST_ENTITY_TOO_LARGE = RESPONSE_CODE(4, 13),
    COAP_UNSUPPORTED_CONTENT_FORMAT = RESPONSE_CODE(4, 15),
    COAP_INTERNAL_SERVER_ERROR = RESPONSE_CODE(5, 0),
    COAP_NOT_IMPLEMENTED = RESPONSE_CODE(5, 1),
    COAP_BAD_GATEWAY = RESPONSE_CODE(5, 2),
    COAP_SERVICE_UNAVALIABLE = RESPONSE_CODE(5, 3),
    COAP_GATEWAY_TIMEOUT = RESPONSE_CODE(5, 4),
    COAP_PROXYING_NOT_SUPPORTED = RESPONSE_CODE(5, 5)
} COAP_RESPONSE_CODE;

typedef enum {
    COAP_IF_MATCH = 1,
    COAP_URI_HOST = 3,
    COAP_E_TAG = 4,
    COAP_IF_NONE_MATCH = 5,
    COAP_URI_PORT = 7,
    COAP_LOCATION_PATH = 8,
    COAP_URI_PATH = 11,
    COAP_CONTENT_FORMAT = 12,
    COAP_MAX_AGE = 14,
    COAP_URI_QUERY = 15,
    COAP_ACCEPT = 17,
    COAP_LOCATION_QUERY = 20,
    COAP_PROXY_URI = 35,
    COAP_PROXY_SCHEME = 39
} COAP_OPTION_NUMBER;

typedef enum {
    COAP_NONE = -1,
    COAP_TEXT_PLAIN = 0,
    COAP_APPLICATION_LINK_FORMAT = 40,
    COAP_APPLICATION_XML = 41,
    COAP_APPLICATION_OCTET_STREAM = 42,
    COAP_APPLICATION_EXI = 47,
    COAP_APPLICATION_JSON = 50
} COAP_CONTENT_TYPE;

typedef struct CoapOption {
	uint8_t Number;
	uint8_t Length;
	uint8_t *Buffer;
} CoapOption;

typedef struct CoapPacket {
    uint8_t Type;
    uint8_t Code;
    uint8_t *Token;
    uint8_t Tokenlen;
    uint8_t *Payload;
    uint8_t Payloadlen;
    uint16_t MessageID;

    uint8_t OptionNum;
    CoapOption Options[MAX_OPTION_NUM];
} CoapPacket;

typedef void (*callback)(CoapPacket*, char*, int);

typedef struct {
	char U[MAX_CALLBACK][300];
	callback C[MAX_CALLBACK];
} CoapUri;

typedef struct CoAPClient {
	CoapUri URI;
	callback ResponseCallback;
	int Port;
	UDPConnection* UDP;
	DNSClient DNS;
} CoAPClient;

// setup
bool CoAPInit(CoAPClient* ap, UDPConnection* udp, UDPConnection* dns_udp);
bool CoAPStart(CoAPClient* ap);
bool CoAPStartPort(CoAPClient* ap, int port);
bool CoAPStop(CoAPClient* ap);

// handler
bool CoAPSetResponseCallback(CoAPClient* ap, callback c);
uint16_t CoAPGet(CoAPClient* ap, char *ip, int port, char *url);
uint16_t CoAPPut(CoAPClient* ap, char *ip, int port, char *url, char* payload);

bool CoAPIsReboot(CoAPClient* ap);
bool CoAPLoop(CoAPClient* ap);

#endif