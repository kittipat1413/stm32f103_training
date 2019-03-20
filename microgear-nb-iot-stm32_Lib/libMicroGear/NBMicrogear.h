#ifndef NBMicrogear_H_
#define NBMicrogear_H_

#define MICROGEARNB_USE_EXTERNAL_BUFFER     0
#define MICROGEARNB_DATA_BUFFER_SIZE        128
#define GWHOST                             "coap.netpie.io"
#define GWPORT                             45683

#include "NBCoAP.h"
#include <stdlib.h>

typedef struct Microgear {
	UDPConnection* UDP;
	UDPConnection* DNS_UDP;
	CoAPClient CoAP;

	char buffer[MICROGEARNB_DATA_BUFFER_SIZE];
	#define buffersize MICROGEARNB_DATA_BUFFER_SIZE
	char *appid;
	char *key;
	char *secret;
	
	char gwaddr[20];
} Microgear;

bool MicrogearInit(Microgear* mg, UDPConnection* udp, UDPConnection* dns_udp, char *host, char *appid, char *key, char *secret);
uint16_t MicrogearSend(Microgear *mg, char *buffer, char* payload);
uint16_t MicrogearPublishString(Microgear *mg, char *topic, char *payload);
uint16_t MicrogearPublishInt(Microgear *mg, char *topic, int value);
void MicrogearChat(Microgear *mg, char *alias, char *payload);
void MicrogearWriteFeed(Microgear *mg, char *feedid, char *payload);
void MicrogearWriteFeedAPIkey(Microgear *mg, char *feedid, char *payload, char *apikey);
void MicrogearPushOwner(Microgear *mg, char *text);
void Microgear_loop(Microgear *mg);
bool MicrogearIsReboot(Microgear *mg);
#endif