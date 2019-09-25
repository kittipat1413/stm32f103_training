#ifndef COAPPACKET_H_
#define COAPPACKET_H_

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

uint16_t CoAPBuildPacket(uint16_t* message_id, uint8_t* buffer, char *url, COAP_TYPE type, COAP_METHOD method, uint8_t* token, uint8_t tokenlen, uint8_t* payload, uint16_t payloadlen);
int CoAPParseOption(CoapOption *option, uint16_t *running_delta, uint8_t **buf, size_t buflen);
bool CoAPDecode(uint16_t message_id, uint8_t* packet, uint16_t packet_length, uint8_t* return_code, uint8_t* payload, uint16_t* payload_length);

uint16_t CoAPBuildPacket(uint16_t* message_id, uint8_t* buffer, char *url, COAP_TYPE type, COAP_METHOD method, uint8_t* token, uint8_t tokenlen, uint8_t* payload, uint16_t payloadlen){
	uint8_t *p = buffer;
	uint16_t packetSize = 0;
	uint16_t running_delta = 0;

	//make header
	*p = 0x01 << 6;
	*p |= (type & 0x03) << 4;
	*p++ |= (tokenlen & 0x0F);
	*p++ = method;
	uint16_t messageID = rand();
	*p++ = messageID >> 8;
	*p++ = messageID & 0xFF;
	p = buffer + COAP_HEADER_SIZE;
	packetSize += 4;

	//make token
	if(token != NULL && tokenlen <= 0x0F){
		memcpy(p, token, tokenlen);
		p += tokenlen;
		packetSize += tokenlen;
	}

	
	//coap option
	//parse url
	CoapOption Options[MAX_OPTION_NUM];
	uint8_t OptionNum = 0;
	uint16_t idx = 0;
    for (uint16_t i = 0; i < strlen(url); i++) {
        if (url[i] == '/') {
            Options[OptionNum].Buffer = (uint8_t *)(url + idx);
            Options[OptionNum].Length = i - idx;
            Options[OptionNum].Number = COAP_URI_PATH;
            OptionNum++;
            idx = i + 1;
        }
    }

	if (idx <= strlen(url)) {
        Options[OptionNum].Buffer = (uint8_t *)(url + idx);
        Options[OptionNum].Length = strlen(url) - idx;
        Options[OptionNum].Number = COAP_URI_PATH;
        OptionNum++;
    }

	//make option
	for(uint8_t i = 0; i < OptionNum; i++){
		uint32_t optdelta;
        uint8_t len, delta;

		if (packetSize + 5 + Options[i].Length >= BUF_MAX_SIZE) {
            return 0;
        }
		optdelta = Options[i].Number - running_delta;
		COAP_OPTION_DELTA(optdelta, &delta);
        COAP_OPTION_DELTA(Options[i].Length, &len);

		*p++ = (0xFF & (delta << 4 | len));
        if (delta == 13) {
            *p++ = (optdelta - 13);
            packetSize++;
        } else if (delta == 14) {
            *p++ = ((optdelta - 269) >> 8);
            *p++ = (0xFF & (optdelta - 269));
            packetSize+=2;
        } if (len == 13) {
            *p++ = (Options[i].Length - 13);
            packetSize++;
        } else if (len == 14) {
            *p++ = (Options[i].Length >> 8);
            *p++ = (0xFF & (Options[i].Length - 269));
            packetSize+=2;
        }

		memcpy(p, Options[i].Buffer, Options[i].Length);
        p += Options[i].Length;
        packetSize += Options[i].Length + 1;
        running_delta = Options[i].Number;
	}

	//make payload
	if (payloadlen > 0) {
        if ((packetSize + 1 + payloadlen) >= BUF_MAX_SIZE) {
            return 0;
        }
        *p++ = 0xFF;
        memcpy(p, payload, payloadlen);
        packetSize += 1 + payloadlen;
    }

	*message_id = messageID;
	return packetSize;
}

int CoAPParseOption(CoapOption *option, uint16_t *running_delta, uint8_t **buf, size_t buflen) {
	uint8_t *p = *buf;
    uint8_t headlen = 1;
    uint16_t len, delta;

    if (buflen < headlen) return -1;

    delta = (p[0] & 0xF0) >> 4;
    len = p[0] & 0x0F;

    if (delta == 13) {
        headlen++;
        if (buflen < headlen) return -1;
        delta = p[1] + 13;
        p++;
    } else if (delta == 14) {
        headlen += 2;
        if (buflen < headlen) return -1;
        delta = ((p[1] << 8) | p[2]) + 269;
        p+=2;
    } else if (delta == 15) return -1;

    if (len == 13) {
        headlen++;
        if (buflen < headlen) return -1;
        len = p[1] + 13;
        p++;
    } else if (len == 14) {
        headlen += 2;
        if (buflen < headlen) return -1;
        len = ((p[1] << 8) | p[2]) + 269;
        p+=2;
    } else if (len == 15)
        return -1;

    if ((p + 1 + len) > (*buf + buflen))  return -1;
    option->Number = delta + *running_delta;
    option->Buffer = p+1;
    option->Length = len;
    *buf = p + 1 + len;
    *running_delta += delta;

    return 0;
}

bool CoAPDecode(uint16_t message_id, uint8_t* packet, uint16_t packet_length, uint8_t* return_code, uint8_t* payload, uint16_t* payload_length){
	// uint8_t type = (packet[0] & 0x30) >> 4;
	uint8_t tokenlen = packet[0] & 0x0F;
	*return_code = packet[1];
	uint16_t MessageID = 0xFF00 & (packet[2] << 8);
    MessageID |= 0x00FF & packet[3];

	// char debug[100];
	// sprintf(debug, "return code: %d.%02d\n", *return_code >> 5, *return_code & 0b00011111);
	// usart_puts1(debug);

	uint8_t* p=packet;
	p = p + 4+tokenlen;
	CoapOption Options[MAX_OPTION_NUM];
	//option/payload
	if(COAP_HEADER_SIZE + tokenlen < packet_length){
		int optionIndex = 0;
        uint16_t delta = 0;
        uint8_t *end = packet + packet_length;
        uint8_t *p = packet + COAP_HEADER_SIZE + tokenlen;

		while(optionIndex < MAX_OPTION_NUM && *p != 0xFF && p < end) {
            //packet.options[optionIndex];
            // NBUartSendDEBUG(ap->UDP->NB, "\r\nCoAPLoop: parseOption");
            if (0 != CoAPParseOption(&Options[optionIndex], &delta, &p, end-p))
                return false;
            optionIndex++;
		}
		
		// sprintf(debug, "option num: %d\n", OptionNum);
		// usart_puts1(debug);

		// uint16_t shift = 0;
		// for(int i =0;i<OptionNum;i++){
		// 	uint8_t optitonlen = Options[i].Length;
		// 	shift += (uint16_t)optitonlen;
		// 	sprintf(debug, "shift: %d\n", optitonlen+1);
		// 	usart_puts1(debug);
		// }
        
		// sprintf(debug,"payload len expect: %d\n", packet_length-4-tokenlen-shift-1);
		// usart_puts1(debug);

		if (p+1 < end && *p == 0xFF) {
            *payload_length = end-(p+1);
			memcpy(payload, p+1,*payload_length);
        } else {
            payload = NULL;
            *payload_length= 0;
        }
        return true;
	}

	// sprintf(debug, "payloadlen: %d\n", *payload_length);
	// usart_puts1(debug);

	// usart_puts1("payload: ");
	// for(int i=0; i< *payload_length; i++){
	// 	sprintf(debug, "%c", payload[i]);
	// 	usart_puts1(debug);
	// }
	// usart_puts1("\n");
    return false;
}

#endif