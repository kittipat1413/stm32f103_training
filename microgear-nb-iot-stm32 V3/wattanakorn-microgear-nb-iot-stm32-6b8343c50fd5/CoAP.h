#ifndef CoAP_H_
#define CoAP_H_

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
	uint8_t number;
	uint8_t length;
	uint8_t *buffer;
} CoapOption;

typedef struct CoapPacket {
    uint8_t type;
    uint8_t code;
    uint8_t *token;
    uint8_t tokenlen;
    uint8_t *payload;
    uint8_t payloadlen;
    uint16_t messageid;

    uint8_t optionnum;
    CoapOption options[MAX_OPTION_NUM];
} CoapPacket;

typedef void (*callback)(CoapPacket*, char*, int);

typedef struct {
	char u[MAX_CALLBACK][300];
	callback c[MAX_CALLBACK];
} CoapUri;

typedef struct Coap {
	BC95UDP *_udp;
	CoapUri uri;
	callback resp;
	int _port;
} Coap;

// method
void CoAPUri_init(CoapUri *coapUri);
void CoAPUri_add(CoapUri *coapUri, callback call, char* url);
callback CoAPUri_find(CoapUri *coapUri, char* url);

void CoAP_init(Coap *coap, BC95UDP *udp);
bool CoAP_start(Coap *coap);
bool CoAP_start_port(Coap *coap, int port);
void CoAP_response(Coap *coap, callback c);
void CoAP_server(Coap *coap, callback c, char* url);
uint16_t CoAP_sendPacket(Coap *coap, CoapPacket *packet, char* ip);
uint16_t CoAP_sendPacket_port(Coap *coap, CoapPacket *packet, char* ip, int port);
uint16_t CoAP_get(Coap *coap, char *ip, int port, char *url);
uint16_t CoAP_put(Coap *coap, char *ip, int port, char *url, char *payload);

uint16_t CoAP_send(Coap *coap, char *host, int port, char *url, COAP_TYPE type, COAP_METHOD method, uint8_t *token, uint8_t tokenlen, uint8_t *payload, uint32_t payloadlen);
int CoAP_parseOption(Coap *coap, CoapOption *option, uint16_t *running_delta, uint8_t **buf, size_t buflen);
uint16_t CoAP_sendResponse(Coap *coap, char *ip, int port, uint16_t messageid, char *payload, int payloadlen, 
	COAP_RESPONSE_CODE code, COAP_CONTENT_TYPE type, uint8_t *token, int tokenlen);
bool CoAP_loop(Coap *coap);
#endif
