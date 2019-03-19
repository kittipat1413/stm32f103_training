#include "Dns.h"
#include "CoAP.h"
#include <string.h>

void CoAP_init(Coap *coap, BC95UDP *udp){
	coap->_udp = udp;
	coap->resp = NULL;
}

bool CoAP_start(Coap *coap) {
	CoAP_start_port(coap, COAP_DEFAULT_PORT);
	return true;
}

bool CoAP_start_port(Coap *coap, int port) {	
	BC95Udp_begin(coap->_udp, port);
	return true;
}

void CoAP_response(Coap *coap, callback c) {
	coap->resp = c;
}

void CoAP_server(Coap *coap, callback c, char* url) {
	CoAPUri_add(&(coap->uri), c, url);
}

uint16_t CoAP_sendPacket(Coap *coap, CoapPacket *packet, char* ip) {
	return CoAP_sendPacket_port(coap, packet, ip, COAP_DEFAULT_PORT);
}

uint16_t CoAP_sendPacket_port(Coap *coap, CoapPacket *packet, char* ip, int port) {
	uint8_t buffer[BUF_MAX_SIZE];
    uint8_t *p = buffer;
    uint16_t running_delta = 0;
    uint16_t packetSize = 0;

    // make coap packet base header
    *p = 0x01 << 6;
    *p |= (packet->type & 0x03) << 4;
    *p++ |= (packet->tokenlen & 0x0F);
    *p++ = packet->code;
    *p++ = (packet->messageid >> 8);
    *p++ = (packet->messageid & 0xFF);
    p = buffer + COAP_HEADER_SIZE;
    packetSize += 4;

    // make token
    if (packet->token != NULL && packet->tokenlen <= 0x0F) {
        memcpy(p, packet->token, packet->tokenlen);
        p += packet->tokenlen;
        packetSize += packet->tokenlen;
    }

    // make option header
    for (int i = 0; i < packet->optionnum; i++)  {
        uint32_t optdelta;
        uint8_t len, delta;

        if (packetSize + 5 + packet->options[i].length >= BUF_MAX_SIZE) {
            return 0;
        }
        optdelta = packet->options[i].number - running_delta;
        COAP_OPTION_DELTA(optdelta, &delta);
        COAP_OPTION_DELTA((uint8_t)packet->options[i].length, &len);

        *p++ = (0xFF & (delta << 4 | len));
        if (delta == 13) {
            *p++ = (optdelta - 13);
            packetSize++;
        } else if (delta == 14) {
            *p++ = ((optdelta - 269) >> 8);
            *p++ = (0xFF & (optdelta - 269));
            packetSize+=2;
        } if (len == 13) {
            *p++ = (packet->options[i].length - 13);
            packetSize++;
        } else if (len == 14) {
            *p++ = (packet->options[i].length >> 8);
            *p++ = (0xFF & (packet->options[i].length - 269));
            packetSize+=2;
        }

        memcpy(p, packet->options[i].buffer, packet->options[i].length);
        p += packet->options[i].length;
        packetSize += packet->options[i].length + 1;
        running_delta = packet->options[i].number;
    }

    // make payload
    if (packet->payloadlen > 0) {
        if ((packetSize + 1 + packet->payloadlen) >= BUF_MAX_SIZE) {
            return 0;
        }
        *p++ = 0xFF;
        memcpy(p, packet->payload, packet->payloadlen);
        packetSize += 1 + packet->payloadlen;
    }

    //char cmd[30];
    //sprintf(cmd, "ip is %d.%d.%d.%d\r\n", ip[0], ip[1], ip[2], ip[3]);
    //BC95_usart_puts(cmd, USART2);

    BC95Udp_beginPacket(coap->_udp, ip, port);
    BC95Udp_write(coap->_udp, buffer, packetSize);
	BC95Udp_endPacket(coap->_udp);
	return packet->messageid;
}

uint16_t CoAP_get(Coap *coap, char *ip, int port, char *url) {
	return CoAP_send(coap, ip, port, url, COAP_CON, COAP_GET, NULL, 0, NULL, 0);
}

uint16_t CoAP_put(Coap *coap, char *ip, int port, char *url, char *payload){
	return CoAP_send(coap, ip, port, url, COAP_CON, COAP_PUT, NULL, 0, (uint8_t*)payload, strlen(payload));
}

uint16_t CoAP_send(Coap *coap, char *host, int port, char *url, COAP_TYPE type, COAP_METHOD method, uint8_t *token, uint8_t tokenlen, uint8_t *payload, uint32_t payloadlen) {
	int ret;
	DNS_CLIENT dns;
	char remote_addr[8];
	char *premote_addr;
	//char cmd[1000];

	DNS_CLIENT_init(&dns, coap->_udp->bc95);
	if (coap->_udp->bc95 == NULL) {
		//sprintf(cmd, "NULL bc95\r\n");
    	//BC95_usart_puts(cmd, USART2);
		return 0;
	}
	//sprintf(cmd, "DNS init OK..\r\n");
    //BC95_usart_puts(cmd, USART2);

    //sprintf(cmd, "host is %s\r\n", host);
    //BC95_usart_puts(cmd, USART2);
    premote_addr = remote_addr;
	DNS_CLIENT_begin(&dns, "8.8.8.8");
	ret = DNS_CLIENT_getHostByName(&dns, host, premote_addr);

	sprintf(remote_addr,"%d.%d.%d.%d", remote_addr[0], remote_addr[1], remote_addr[2], remote_addr[3]);

	//sprintf(cmd, "ret is %d %02X.%02X.%02X.%02X\r\n", ret,remote_addr[0],remote_addr[1],remote_addr[2],remote_addr[3]);
    //BC95_usart_puts(cmd, USART2);
	if (!ret){
		return 0;
	}
	
	// make packet
    CoapPacket packet;

    packet.type = type;
    packet.code = method;
    packet.token = token;
    packet.tokenlen = tokenlen;
    packet.payload = payload;
    packet.payloadlen = payloadlen;
    packet.optionnum = 0;
    packet.messageid = rand();

    // use URI_HOST UIR_PATH
    //String ipaddress = String(ip[0]) + String(".") + String(ip[1]) + String(".") + String(ip[2]) + String(".") + String(ip[3]);
    packet.options[packet.optionnum].buffer = (uint8_t *)host;
    packet.options[packet.optionnum].length = 4;
    packet.options[packet.optionnum].number = COAP_URI_HOST;
    packet.optionnum++;

    // parse url
    uint16_t idx = 0;
    for (uint16_t i = 0; i < strlen(url); i++) {
        if (url[i] == '/') {
            packet.options[packet.optionnum].buffer = (uint8_t *)(url + idx);
            packet.options[packet.optionnum].length = i - idx;
            packet.options[packet.optionnum].number = COAP_URI_PATH;
            packet.optionnum++;
            idx = i + 1;
        }
    }

    if (idx <= strlen(url)) {
        packet.options[packet.optionnum].buffer = (uint8_t *)(url + idx);
        packet.options[packet.optionnum].length = strlen(url) - idx;
        packet.options[packet.optionnum].number = COAP_URI_PATH;
        packet.optionnum++;
    }

    return CoAP_sendPacket_port(coap, &packet, remote_addr, port);
    
    //return 0;
}

int CoAP_parseOption(Coap *coap, CoapOption *option, uint16_t *running_delta, uint8_t **buf, size_t buflen) {
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
    option->number = delta + *running_delta;
    option->buffer = p+1;
    option->length = len;
    *buf = p + 1 + len;
    *running_delta += delta;

    return 0;
}

bool CoAP_loop(Coap *coap) {
	uint8_t buffer[BUF_MAX_SIZE];
    //int32_t packetlen = coap->_udp->parsePacket();
	int32_t packetlen = BC95Udp_parsePacket(coap->_udp);

    while (packetlen > 0) {
        //packetlen = coap->_udp->read(buffer, packetlen >= BUF_MAX_SIZE ? BUF_MAX_SIZE : packetlen);
        packetlen = BC95UDP_read(coap->_udp, buffer, packetlen >= BUF_MAX_SIZE ? BUF_MAX_SIZE : packetlen);
        CoapPacket packet;

        // parse coap packet header
        if (packetlen < COAP_HEADER_SIZE || (((buffer[0] & 0xC0) >> 6) != 1)) {
            //packetlen = coap->_udp->parsePacket();
            packetlen = BC95Udp_parsePacket(coap->_udp);
            continue;
        }

        packet.type = (buffer[0] & 0x30) >> 4;
        packet.tokenlen = buffer[0] & 0x0F;
        packet.code = buffer[1];
        packet.messageid = 0xFF00 & (buffer[2] << 8);
        packet.messageid |= 0x00FF & buffer[3];

        if (packet.tokenlen == 0)  packet.token = NULL;
        else if (packet.tokenlen <= 8)  packet.token = buffer + 4;
        else {
            //packetlen = _udp->parsePacket();
            packetlen = BC95Udp_parsePacket(coap->_udp);
            continue;
        }

        // parse packet options/payload
        if (COAP_HEADER_SIZE + packet.tokenlen < packetlen) {
            int optionIndex = 0;
            uint16_t delta = 0;
            uint8_t *end = buffer + packetlen;
            uint8_t *p = buffer + COAP_HEADER_SIZE + packet.tokenlen;
            while(optionIndex < MAX_OPTION_NUM && *p != 0xFF && p < end) {
                //packet.options[optionIndex];
                if (0 != CoAP_parseOption(coap, &packet.options[optionIndex], &delta, &p, end-p))
                    return false;
                optionIndex++;
            }
            packet.optionnum = optionIndex;

            if (p+1 < end && *p == 0xFF) {
                packet.payload = p+1;
                packet.payloadlen = end-(p+1);
            } else {
                packet.payload = NULL;
                packet.payloadlen= 0;
            }
        }

        if (packet.type == COAP_ACK) {
            // call response function
            #if COAP_ENABLE_ACK_CALLBACK == 1
                if (coap->resp != NULL){
                    //coap->resp(packet, coap->_udp->remoteIP(), _udp->remotePort());
                    coap->resp(&packet, BC95UDP_remoteIP(coap->_udp), BC95UDP_remotePort(coap->_udp));
                }
            #endif
        } else {
            // e.g. packet.type == COAP_CON

            char url[300] = "";
            // call endpoint url function
            for (int i = 0; i < packet.optionnum; i++) {
                if (packet.options[i].number == COAP_URI_PATH && packet.options[i].length > 0) {
                    char urlname[packet.options[i].length + 1];
                    memcpy(urlname, packet.options[i].buffer, packet.options[i].length);
                    urlname[packet.options[i].length] = 0;
                    if(strlen(url) > 0) {
                      	//url += "/";
                  		strcat(url, "/");
                  	}
                    //url += urlname;
                    strcat(url, urlname);
                }
            }
            
            //if (!uri.find(url)) {
            if (!CoAPUri_find(&(coap->uri), url)){
                // sendResponse(_udp->remoteIP(), _udp->remotePort(), packet.messageid, NULL, 0,
                // COAP_NOT_FOUNT, COAP_NONE, NULL, 0);
                CoAP_sendResponse(coap, BC95UDP_remoteIP(coap->_udp), BC95UDP_remotePort(coap->_udp), packet.messageid, NULL, 0, 
					COAP_NOT_FOUNT, COAP_NONE, NULL, 0);
            } else {
                //uri.find(url)(packet, _udp->remoteIP(), _udp->remotePort());
                callback c = CoAPUri_find(&(coap->uri), url);
                c(&packet, BC95UDP_remoteIP(coap->_udp), BC95UDP_remotePort(coap->_udp));
                //(packet, BC95UDP_remoteIP(coap->_udp), BC95UDP_remotePort(coap->_udp));
                // ack here?
                // sendResponse(_udp->remoteIP(), _udp->remotePort(), packet.messageid);
            }
            
        }

        // next packet
        //packetlen = _udp->parsePacket();
        packetlen = BC95Udp_parsePacket(coap->_udp);
    }

    return true;
}

uint16_t CoAP_sendResponse(Coap *coap, char *ip, int port, uint16_t messageid, char *payload, int payloadlen, 
	COAP_RESPONSE_CODE code, COAP_CONTENT_TYPE type, uint8_t *token, int tokenlen) {
	// make packet
    CoapPacket packet;

    packet.type = COAP_ACK;
    packet.code = code;
    packet.token = token;
    packet.tokenlen = tokenlen;
    packet.payload = (uint8_t *)payload;
    packet.payloadlen = payloadlen;
    packet.optionnum = 0;
    packet.messageid = messageid;

    // if more options?
    char optionBuffer[2];
    optionBuffer[0] = ((uint16_t)type & 0xFF00) >> 8;
    optionBuffer[1] = ((uint16_t)type & 0x00FF) ;
    packet.options[packet.optionnum].buffer = (uint8_t *)optionBuffer;
    packet.options[packet.optionnum].length = 2;
    packet.options[packet.optionnum].number = COAP_CONTENT_FORMAT;
    packet.optionnum++;

    return CoAP_sendPacket_port(coap, &packet, ip, port);
}

void CoAPUri_init(CoapUri *coapUri) {
	for (int i = 0; i < MAX_CALLBACK; i++) {
        //coapUri->u[i] = "";
        memset (coapUri->u[i],0,300);
        coapUri->c[i] = NULL;
        //memset (coapUri->c[i],NULL,300);
    }
}

void CoAPUri_add(CoapUri *coapUri, callback call, char* url) {
	for (int i = 0; i < MAX_CALLBACK; i++)
        //if (coapUri->c[i] != NULL && coapUri->u[i].equals(url)) {
        if (coapUri->c[i] != NULL && strcmp(coapUri->u[i],url)) {
            coapUri->c[i] = call;
            return ;
        }
    for (int i = 0; i < MAX_CALLBACK; i++) {
        if (coapUri->c[i] == NULL) {
            coapUri->c[i] = call;
            //coapUri->u[i] = url;
            strcpy(coapUri->u[i], url);
            return;
        }
    }
}

callback CoAPUri_find(CoapUri *coapUri, char* url) {
	for (int i = 0; i < MAX_CALLBACK; i++) {
		//if (c[i] != NULL && u[i].equals(url)) {
		if (coapUri->c[i] != NULL && strcmp(coapUri->u[i],url) ) {
			return coapUri->c[i];
		} 
	}
    return NULL;
}