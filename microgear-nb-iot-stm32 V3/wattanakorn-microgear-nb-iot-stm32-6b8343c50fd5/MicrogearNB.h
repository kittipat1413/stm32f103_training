#ifndef MicrogearNB_H_
#define MicrogearNB_H_

#define MICROGEARNB_USE_EXTERNAL_BUFFER     0
#define MICROGEARNB_DATA_BUFFER_SIZE        128
#define GWHOST                             "coap.netpie.io"
#define GWPORT                             45683

#include "CoAP.h"
#include <stdlib.h>

typedef struct Microgear {
	BC95UDP *udp;
	Coap *coap;

	char buffer[MICROGEARNB_DATA_BUFFER_SIZE];
	#define buffersize MICROGEARNB_DATA_BUFFER_SIZE
	char *appid;
	char *key;
	char *secret;
	char *gwaddr;
} Microgear;

void Microgear_udp_init(Microgear *mg, BC95UDP *udp);
void Microgear_init(Microgear *mg, char *appid, char *key, char *secret);
void Microgear_begin(Microgear *mg, uint16_t port);
void Microgear_publish_int(Microgear *mg, char *topic, int value);
void Microgear_publish_string(Microgear *mg, char *topic, char *payload);
void Microgear_chat(Microgear *mg, char *alias, char *payload);
void Microgear_writeFeed(Microgear *mg, char *feedid, char *payload);
void Microgear_writeFeed_APIkey(Microgear *mg, char *feedid, char *payload, char *apikey);
void Microgear_pushOwner(Microgear *mg, char *text);
void Microgear_loop(Microgear *mg);

void Microgear_coapSend(Microgear *mg, char *buffer, char* payload);
#endif