#include "NBDNS.h"
#include "NBCoAP.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

bool CoAPUriInit(CoapUri* coapUri){
	for (int i = 0; i < MAX_CALLBACK; i++) {
        //coapUri->u[i] = "";
        memset (coapUri->U[i],0,300);
        coapUri->C[i] = NULL;
        //memset (coapUri->c[i],NULL,300);
    }
    return true;
}

bool CoAPInit(CoAPClient* ap, UDPConnection* udp, UDPConnection* dns_udp){
	if(ap == NULL){
		return false;
	}

	if(udp == NULL){
		return false;
	}

	ap->UDP = udp;
	ap->ResponseCallback = NULL;
	ap->Port = -1;

	// init DNS
	bool ret = DNSInit(&(ap->DNS), dns_udp);
	if(ret){
		return true;
	}else{
		return false;
	}

	ret = CoAPUriInit(&ap->URI);
	if(ret){
		return true;
	}else{
		return false;
	}

	return true;
}

bool CoAPStartPort(CoAPClient* ap, int port){
	bool ret = UDPConnect(ap->UDP, NULL, (uint16_t)port );
	return ret;
}

bool CoAPStart(CoAPClient* ap){
	return CoAPStartPort(ap, COAP_DEFAULT_PORT);
}

bool CoAPStop(CoAPClient* ap){
	return UDPDisconnect(ap->UDP);
}

bool CoAPSetResponseCallback(CoAPClient* ap, callback c){
	ap->ResponseCallback = c;
	return true;
}

uint16_t CoAPSendPacket(CoAPClient* ap, CoapPacket *packet, char* HostIP, int port){
	uint8_t buffer[BUF_MAX_SIZE];
    uint8_t *p = buffer;
    uint16_t running_delta = 0;
    uint16_t packetSize = 0;

    // make coap packet base header
    *p = 0x01 << 6;
    *p |= (packet->Type & 0x03) << 4;
    *p++ |= (packet->Tokenlen & 0x0F);
    *p++ = packet->Code;
    *p++ = (packet->MessageID >> 8);
    *p++ = (packet->MessageID & 0xFF);
    p = buffer + COAP_HEADER_SIZE;
    packetSize += 4;

    // make token
    if (packet->Token != NULL && packet->Tokenlen <= 0x0F) {
        memcpy(p, packet->Token, packet->Tokenlen);
        p += packet->Tokenlen;
        packetSize += packet->Tokenlen;
    }

    // make option header
    for (int i = 0; i < packet->OptionNum; i++)  {
        uint32_t optdelta;
        uint8_t len, delta;

        if (packetSize + 5 + packet->Options[i].Length >= BUF_MAX_SIZE) {
            return 0;
        }
        optdelta = packet->Options[i].Number - running_delta;
        COAP_OPTION_DELTA(optdelta, &delta);
        COAP_OPTION_DELTA((uint8_t)packet->Options[i].Length, &len);

        *p++ = (0xFF & (delta << 4 | len));
        if (delta == 13) {
            *p++ = (optdelta - 13);
            packetSize++;
        } else if (delta == 14) {
            *p++ = ((optdelta - 269) >> 8);
            *p++ = (0xFF & (optdelta - 269));
            packetSize+=2;
        } if (len == 13) {
            *p++ = (packet->Options[i].Length - 13);
            packetSize++;
        } else if (len == 14) {
            *p++ = (packet->Options[i].Length >> 8);
            *p++ = (0xFF & (packet->Options[i].Length - 269));
            packetSize+=2;
        }

        memcpy(p, packet->Options[i].Buffer, packet->Options[i].Length);
        p += packet->Options[i].Length;
        packetSize += packet->Options[i].Length + 1;
        running_delta = packet->Options[i].Number;
    }

    // make payload
    if (packet->Payloadlen > 0) {
        if ((packetSize + 1 + packet->Payloadlen) >= BUF_MAX_SIZE) {
            return 0;
        }
        *p++ = 0xFF;
        memcpy(p, packet->Payload, packet->Payloadlen);
        packetSize += 1 + packet->Payloadlen;
    }

    UDPFlushOutbound(ap->UDP);
    UDPWrite(ap->UDP, buffer, packetSize);
    char outbuf[200];
    memcpy(outbuf, buffer, packetSize);
    char *ptr;
    ptr = outbuf + packetSize;
    *ptr = '\0';
    // NBUartSendDEBUG(ap->UDP->NB, "\r\n buffer: ");
    // NBUartSendDEBUG(ap->UDP->NB, outbuf);
    // NBUartSendDEBUG(ap->UDP->NB, "\r\n");
    // Set Destination IP Port
    bool pk;
	pk = UDPSetIP(ap->UDP, HostIP);
	pk = UDPSetPort(ap->UDP, (uint16_t)port);
	if(!pk){
		return 0;
	}

    UDPSendPacket(ap->UDP);
    // NBUartSendDEBUG(ap->UDP->NB,out);

    UDPFlush(ap->UDP);

	return packet->MessageID;
}

uint16_t CoAPSend(CoAPClient* ap, char *hostname, int port, char *url, COAP_TYPE type, COAP_METHOD method, uint8_t *token, uint8_t tokenlen, uint8_t *payload, uint32_t payloadlen){
	int ret;
	char HostIP[20];

	// NBUartSendDEBUG(ap->UDP->NB,"\r\nDNSSolve\r\n");
	// NBUartSendDEBUG(ap->UDP->NB, hostname);
	ret = DNSSolve(&ap->DNS, (const char*)hostname, HostIP);
	if(!ret){
		// NBUartSendDEBUG(ap->UDP->NB,"\r\nDNSSolve fail\r\n");
		return 0;
	}
	
	// NBUartSendDEBUG(ap->UDP->NB,"\r\nHostIP: ");
	// NBUartSendDEBUG(ap->UDP->NB, HostIP);
	// NBUartSendDEBUG(ap->UDP->NB,"\r\n");
	// make packet
	CoapPacket packet;

    packet.Type = type;
    packet.Code = method;
    packet.Token = token;
    packet.Tokenlen = tokenlen;
    packet.Payload = payload;
    packet.Payloadlen = payloadlen;
    packet.OptionNum = 0;
    packet.MessageID = rand();

    // NBUartSendDEBUG();
    // use URI_HOST UIR_PATH
    packet.Options[packet.OptionNum].Buffer = (uint8_t *)hostname;
    packet.Options[packet.OptionNum].Length = 4;
    packet.Options[packet.OptionNum].Number = COAP_URI_HOST;
    packet.OptionNum++;

    // parse url
    uint16_t idx = 0;
    for (uint16_t i = 0; i < strlen(url); i++) {
        if (url[i] == '/') {
            packet.Options[packet.OptionNum].Buffer = (uint8_t *)(url + idx);
            packet.Options[packet.OptionNum].Length = i - idx;
            packet.Options[packet.OptionNum].Number = COAP_URI_PATH;
            packet.OptionNum++;
            idx = i + 1;
        }
    }

    if (idx <= strlen(url)) {
        packet.Options[packet.OptionNum].Buffer = (uint8_t *)(url + idx);
        packet.Options[packet.OptionNum].Length = strlen(url) - idx;
        packet.Options[packet.OptionNum].Number = COAP_URI_PATH;
        packet.OptionNum++;
    }
    return CoAPSendPacket(ap, &packet, HostIP, port); 
	//return 0;
}

uint16_t CoAPGet(CoAPClient* ap, char *ip, int port, char *url){
	return CoAPSend(ap, ip, port, url, COAP_CON, COAP_GET, NULL, 0, NULL, 0);
}

uint16_t CoAPPut(CoAPClient* ap, char *ip, int port, char *url, char *payload){
	// NBUartSendDEBUG(ap->UDP->NB,"\r\nCoAPPut: ");
	// NBUartSendDEBUG(ap->UDP->NB, ip);
	// NBUartSendDEBUG(ap->UDP->NB,"\r\n");
	return CoAPSend(ap, ip, port, url, COAP_CON, COAP_PUT, NULL, 0, (uint8_t*)payload, strlen(payload));
}

int CoAPParseOption(CoAPClient* ap, CoapOption *option, uint16_t *running_delta, uint8_t **buf, size_t buflen) {
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

callback CoAPUriFind(CoapUri *coapUri, char* url){
	for (int i = 0; i < MAX_CALLBACK; i++) {
		//if (c[i] != NULL && u[i].equals(url)) {
		if (coapUri->C[i] != NULL && strcmp(coapUri->U[i],url) ) {
			return coapUri->C[i];
		} 
	}
    return NULL;
}

uint16_t CoAPSendResponse(CoAPClient* ap, char *ip, int port, uint16_t messageid, char *payload, int payloadlen, 
	COAP_RESPONSE_CODE code, COAP_CONTENT_TYPE type, uint8_t *token, int tokenlen) {
	// make packet
    CoapPacket packet;

    packet.Type = COAP_ACK;
    packet.Code = code;
    packet.Token = token;
    packet.Tokenlen = tokenlen;
    packet.Payload = (uint8_t *)payload;
    packet.Payloadlen = payloadlen;
    packet.OptionNum = 0;
    packet.MessageID = messageid;

    // if more options?
    char optionBuffer[2];
    optionBuffer[0] = ((uint16_t)type & 0xFF00) >> 8;
    optionBuffer[1] = ((uint16_t)type & 0x00FF) ;
    packet.Options[packet.OptionNum].Buffer = (uint8_t *)optionBuffer;
    packet.Options[packet.OptionNum].Length = 2;
    packet.Options[packet.OptionNum].Number = COAP_CONTENT_FORMAT;
    packet.OptionNum++;

    return CoAPSendPacket(ap, &packet, ip, port);
}

bool CoAPIsReboot(CoAPClient* ap){
    return UDPIsReboot(ap->UDP);
}

bool CoAPLoop(CoAPClient* ap){
	uint8_t buffer[BUF_MAX_SIZE];    

	//NBUartSendDEBUG(ap->UDP->NB, "\r\nCoAPLoop: UDPParsePacket");
	int32_t packetlen = UDPParsePacket(ap->UDP);

	//char out[100];
    while (packetlen > 0) {
        //packetlen = coap->_udp->read(buffer, packetlen >= BUF_MAX_SIZE ? BUF_MAX_SIZE : packetlen);
        // NBUartSendDEBUG(ap->UDP->NB, "\r\nCoAPLoop: UDPRead");
        packetlen = UDPRead(ap->UDP, buffer, packetlen >= BUF_MAX_SIZE ? BUF_MAX_SIZE : packetlen);
        // sprintf(out, "\r\npacketlen: %ld", packetlen);
        // NBUartSendDEBUG(ap->UDP->NB, out);

        CoapPacket packet;

        // parse coap packet header
        if (packetlen < COAP_HEADER_SIZE || (((buffer[0] & 0xC0) >> 6) != 1)) {
            //packetlen = coap->_udp->parsePacket();

            // NBUartSendDEBUG(ap->UDP->NB, "\r\nCoAPLoop: UDPParsePacket header");
            packetlen = UDPParsePacket(ap->UDP);

         //    sprintf(out, "\r\nheader packetlen: %ld", packetlen);
       		// NBUartSendDEBUG(ap->UDP->NB, out);
            continue;
        }

        packet.Type = (buffer[0] & 0x30) >> 4;
        packet.Tokenlen = buffer[0] & 0x0F;
        packet.Code = buffer[1];
        packet.MessageID = 0xFF00 & (buffer[2] << 8);
        packet.MessageID |= 0x00FF & buffer[3];

        if (packet.Tokenlen == 0)  packet.Token = NULL;
        else if (packet.Tokenlen <= 8)  packet.Token = buffer + 4;
        else {
            //packetlen = _udp->parsePacket();
            // NBUartSendDEBUG(ap->UDP->NB, "\r\nCoAPLoop: UDPParsePacket token");
            packetlen = UDPParsePacket(ap->UDP);
            continue;
        }

        // parse packet options/payload
        if (COAP_HEADER_SIZE + packet.Tokenlen < packetlen) {
            int optionIndex = 0;
            uint16_t delta = 0;
            uint8_t *end = buffer + packetlen;
            uint8_t *p = buffer + COAP_HEADER_SIZE + packet.Tokenlen;
            while(optionIndex < MAX_OPTION_NUM && *p != 0xFF && p < end) {
                //packet.options[optionIndex];
                // NBUartSendDEBUG(ap->UDP->NB, "\r\nCoAPLoop: parseOption");
                if (0 != CoAPParseOption(ap, &packet.Options[optionIndex], &delta, &p, end-p))
                    return false;
                optionIndex++;
            }
            packet.OptionNum = optionIndex;

            if (p+1 < end && *p == 0xFF) {
                packet.Payload = p+1;
                packet.Payloadlen = end-(p+1);
            } else {
                packet.Payload = NULL;
                packet.Payloadlen= 0;
            }
        }

        if (packet.Type == COAP_ACK) {
            // call response function
            if (ap->ResponseCallback != NULL){

            	//NBUartSendDEBUG(ap->UDP->NB, "\r\nCoAPLoop: Call ResponseCallback\r\n");
                //coap->resp(packet, coap->_udp->remoteIP(), _udp->remotePort());
                ap->ResponseCallback(&packet, UDPGetIP(ap->UDP), UDPGetPort(ap->UDP));
            }
        } else {
            // e.g. packet.type == COAP_CON

            char url[300] = "";
            // call endpoint url function
            for (int i = 0; i < packet.OptionNum; i++) {
                if (packet.Options[i].Number == COAP_URI_PATH && packet.Options[i].Length > 0) {
                    char urlname[packet.Options[i].Length + 1];
                    memcpy(urlname, packet.Options[i].Buffer, packet.Options[i].Length);
                    urlname[packet.Options[i].Length] = 0;
                    if(strlen(url) > 0) {
                      	//url += "/";
                  		strcat(url, "/");
                  	}
                    //url += urlname;
                    strcat(url, urlname);
                }
            }
            
            //if (!uri.find(url)) {
            if (!CoAPUriFind(&(ap->URI), url)){
                // sendResponse(_udp->remoteIP(), _udp->remotePort(), packet.messageid, NULL, 0,
                // COAP_NOT_FOUNT, COAP_NONE, NULL, 0);
                CoAPSendResponse(ap, UDPGetIP(ap->UDP), UDPGetPort(ap->UDP), packet.MessageID, NULL, 0, 
					COAP_NOT_FOUNT, COAP_NONE, NULL, 0);
            } else {
                //uri.find(url)(packet, _udp->remoteIP(), _udp->remotePort());
                callback c = CoAPUriFind(&(ap->URI), url);
                c(&packet, UDPGetIP(ap->UDP), UDPGetPort(ap->UDP));
                //(packet, BC95UDP_remoteIP(coap->_udp), BC95UDP_remotePort(coap->_udp));
                // ack here?
                // sendResponse(_udp->remoteIP(), _udp->remotePort(), packet.messageid);
            }
            
        }

        // next packet
        //packetlen = _udp->parsePacket();
        packetlen = UDPParsePacket(ap->UDP);
    }

    return true;
}