#ifndef BC95Udp_H_
#define BC95Udp_H_

#include "BC95.h"
#include <stdlib.h>

#define pbuffersize BC95UDP_BUFFER_SIZE

typedef struct BC95UDP {
	BC95_str *bc95;
	char *dip;
	uint16_t dport;
	SOCKD *socket;
	uint8_t *pktcur;	
    uint8_t pbuffer[BC95UDP_BUFFER_SIZE];
    size_t pbufferlen;
} BC95UDP;

uint8_t BC95Udp_init(BC95UDP* bc95udp, BC95_str* bc95);
uint8_t BC95Udp_begin(BC95UDP* bc95udp, uint16_t port);
void BC95Udp_stop(BC95UDP* bc95udp);
int BC95Udp_beginPacket(BC95UDP* bc95udp, char *ip, uint16_t port);
size_t BC95Udp_write(BC95UDP* bc95udp, const uint8_t* buffer, size_t size);
char* BC95Udp_endPacket(BC95UDP* bc95udp);
int BC95Udp_parsePacket(BC95UDP* bc95udp);
int BC95UDP_available(BC95UDP* bc95udp);
int BC95UDP_read_null(BC95UDP* bc95udp);
int BC95UDP_read(BC95UDP* bc95udp, uint8_t* buffer, size_t len);
int BC95UDP_peek(BC95UDP* bc95udp);
void BC95UDP_flush(BC95UDP* bc95udp);
char* BC95UDP_remoteIP(BC95UDP* bc95udp);
uint16_t BC95UDP_remotePort(BC95UDP* bc95udp);

#endif