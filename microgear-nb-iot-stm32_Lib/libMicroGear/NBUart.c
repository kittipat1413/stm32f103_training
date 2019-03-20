#include "NBUart.h"
const char STOPPER[][STOPPERLEN] = {END_LINE, END_OK, END_ERROR};

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define STOPPERCOUNT COUNT_OF(STOPPER)
#define MAXSOCKET 7

static inline void Delay_1us(uint32_t nCnt_1us)
{
  volatile uint32_t nCnt;

  for (; nCnt_1us != 0; nCnt_1us--)
    for (nCnt = 13; nCnt != 0; nCnt--);
}

void NBUart_send_byte(uint8_t b, USART_TypeDef* usartx){
	/* Send one byte */
	USART_SendData(usartx, b);

	/* Loop until USART2 DR register is empty */
	while (USART_GetFlagStatus(usartx, USART_FLAG_TXE) == RESET);
}

void NBUartInit(NBUart* nb, USART_TypeDef* usartx, NBQueue* q){
	nb->USART = usartx;
	nb->InboundQueue = q;
	memset(nb->buffer, 0, BUFFER_SIZE);

	nb->Reboot = false;
}

void NBUartSend(NBUart* nb, char* s){
	while(*s) {
		NBUart_send_byte(*s, nb->USART);
		s++;
	}
}

void NBUartSendDEBUG(NBUart* nb, char* s){
	while(*s) {
		NBUart_send_byte(*s, USART2);
		s++;
	}
}

bool NBUartIsAvailable(NBUart* nb){
	if( !NBQueueIsEmpty(nb->InboundQueue) ){
		// queue has data
		return true;
	}
	return false;
}

uint8_t NBUartReadByte(NBUart* nb){
	return NBQueueRemove(nb->InboundQueue);
}

int8_t NBUartGetResponse(NBUart* nb, uint32_t timeout){
	memset(nb->buffer, 0, BUFFER_SIZE);
	char *p = nb->buffer;

	uint8_t CRcount = 0;
	uint8_t	LFcount = 0;
	uint8_t CRLFcount = 0;

	char out[200];
	int i=0;

	bool reboot = false;

	while(true){
		if(NBUartIsAvailable(nb)){
			*p = (char)NBUartReadByte(nb);

			// sprintf(out, "%02X",*p);
			// NBUartSendDEBUG(nb, out);
			// i++;			

			if( *p != '\r' && p == nb->buffer){
				//ignore
				continue;
			} else if( *p == '\r'){ //CR
				CRLFcount++; 				
			} else if( *p == '\n'){//LF
				CRLFcount++;
			}

			p++;

			if(CRLFcount == 4){
				*p = '\0';
				CRLFcount = 0;

				char *f = NULL;

				//check REBOOT
				f = strstr((const char *)nb->buffer, "REBOOT");
				if(f != NULL){
					// NBUartSendDEBUG(nb, "found reboot\r\n");
					//reset
					p = nb->buffer;
					reboot = true;
					nb->Reboot = true;
					continue;
				}

				if(reboot){
					//reset
					p = nb->buffer;
					reboot = false;
					nb->Reboot = false;
					continue;
				}

				//check NSOMMI
				f = strstr((const char *)nb->buffer, "+NSOMMI:");
				if(f != NULL){
					//reset
					p = nb->buffer;
					continue;
				}

				//check OK
				f = strstr((const char *)nb->buffer, (const char *)END_OK);
				if(f != NULL){
					*(f-2) = '\0';
					uint16_t len = strlen(nb->buffer);
					memmove(nb->buffer,nb->buffer+2, len);
					return READ_COMPLETE_OK;
				}

				//check ERROR
				f = strstr((const char *)nb->buffer, (const char *)END_ERROR);
				if(f != NULL){
					*f = '\0';
					return READ_COMPLETE_ERROR;
				}
			}

		}else{
			if(timeout <= 0){
				*p = '\0';
				memset(nb->buffer, 0, BUFFER_SIZE);				
				return READ_TIMEOUT;
			}
			else{
				Delay_1us(5000);
				timeout--;
				continue;
			}
		}
	}

	NBUartSendDEBUG(nb,"INCOMPLETE: OTHER CAUSE\r\n");
	memset(nb->buffer, 0, BUFFER_SIZE);
	return READ_INCOMPLETE;
}

char* ResponseString(int8_t r){
	if(r == READ_COMPLETE_OK){
		return "READ_COMPLETE_OK";
	}else if(r == READ_COMPLETE_ERROR){
		return "READ_COMPLETE_ERROR";
	}else{
		return "READ_INCOMPLETE";
	}
}

bool NBUartGetString(NBUart* nb, char* dst){
	strcpy(dst, nb->buffer);
	return true;
}

ResponseMessage NBUartParse(NBUart* nb, char *rawstr, char* prefix){
	uint8_t i;
	char *r, *p;

	ResponseMessage rm;

	char *end = NULL;
	end = strstr((const char *)rawstr, (const char *)END_OK);
	if(end != NULL){
		rm.EndingStatus = true;
	} else {
		// end = strstr((const char *)rawstr, (const char *)END_OK2);
		// if(end != NULL){
		// 	rm.EndingStatus = true;
		// } else {
		// 	rm.EndingStatus = false;
		// }
		rm.EndingStatus = false;
	}

	if (prefix == NULL || *prefix=='\0'){
		rm.Message = rawstr;
		return rm;
	}

	//if (prefix == NOMSG){
	if(strcmp(prefix,NOMSG)==0){
		rm.Message = rawstr;
		rm.EndingStatus = true;
		return rm;
	}

	p = strstr((const char *)rawstr, (const char *)prefix);
    if (p != NULL) {
		p += strlen(prefix);
        // skip STOPPER[0]

        for (i=1; i<STOPPERCOUNT; i++) {
            r = strstr((const char *)p, (const char *)STOPPER[i]);
            if (r != NULL) {
                *r = '\0';
    	    	r += strlen(STOPPER[i]);

    	    	rm.Message = p;
                return rm;
            }
        }
	}

	p = NULL;
	p = strstr((const char *)rawstr, "+NSONMI:");
	if(p != NULL){
		// NBUartSendDEBUG(nb, "\r\nNSONMI: ");
		// NBUartSendDEBUG(nb, rawstr);
		// NBUartSendDEBUG(nb, "\r\n");
		rm.Message = NULL;
		rm.EndingStatus = false;
		return rm;
	}
	
	// NBUartSendDEBUG(nb, "FLASE");
	rm.EndingStatus = false;
	return rm;
}

bool NBUartReset(NBUart* nb){
	NBUartSend(nb, "AT+NRB\r\n");
	Delay_1us(2000000);
	int8_t ret = NBUartGetResponse(nb, 100);

	NBUartSend(nb, "AT+CFUN=1\r\n");
	Delay_1us(2000000);

	uint8_t cnt=0;
	repeat:
	NBUartSend(nb, "AT\r\n");
	Delay_1us(2000000);

	ret = NBUartGetResponse(nb, 100);
	if(ret != READ_COMPLETE_OK){
		Delay_1us(2000000);
		cnt++;
		if(cnt == 5){
			Delay_1us(2000000);
			return false;
		}
		goto repeat;
	}

	Delay_1us(2000000);
	return true;
}

bool NBUartGetIMI(NBUart* nb, char* dst){
	NBUartSend(nb, "AT+CIMI\r\n");
	int8_t ret = NBUartGetResponse(nb, 100);
	if(ret== READ_COMPLETE_OK){
		NBUartGetString(nb, dst);
		return true;
	}
	return false;
}

bool NBUartGetManufacturerModel(NBUart* nb, char* dst){
	NBUartSend(nb, "AT+CGMM\r\n");
	int8_t ret = NBUartGetResponse(nb, 100);
	if(ret == READ_COMPLETE_OK){
		NBUartGetString(nb, dst);
		return true;
	}
	return false;
}

bool NBUartGetManufacturerRevision(NBUart* nb, char* dst){
	NBUartSend(nb, "AT+CGMR\r\n");
	int8_t ret = NBUartGetResponse(nb, 100);
	if(ret == READ_COMPLETE_OK){
		NBUartGetString(nb, dst);
		return true;
	}
	return false;
}

bool NBUartGetIPAddress(NBUart* nb, char* dst){
	NBUartSend(nb, "AT+CGPADDR=0\r\n");
	int8_t ret = NBUartGetResponse(nb, 100);
	if(ret == READ_COMPLETE_OK){
		char text[60];
		NBUartGetString(nb, text);
		char *p = strstr(text,"+CGPADDR:0,");
		uint8_t len = strlen("+CGPADDR:0,");
		sprintf(dst,"%s", p+len);
		return true;
	}
	return false;
}

int8_t NBUartGetSignalStrength(NBUart* nb){
	NBUartSend(nb, "AT+CSQ\r\n");
	int8_t ret = NBUartGetResponse(nb, 100);
	if(ret == READ_COMPLETE_OK){
		char text[60];
		NBUartGetString(nb, text);
		NBUartSendDEBUG(nb,text);
		char *p = strstr(text,"+CSQ:");
		uint8_t len = strlen("+CSQ:");
		sprintf(text,"%s", p+len);

		p = text;
		int8_t r = 0, i;
		for (i=0; i<2; i++) {
    	    if (*p>='0' && *p<='9'){
    	    	r=10*r + *p-'0';
   	    	}
    	    p++;
    	}
		return 2*r-113;
	}
	return -114;
}

bool NBUartAttachNetwork(NBUart* nb){
	Delay_1us(2000000);

	NBUartSend(nb, "AT+CGATT=1\r\n");
	NBUartGetResponse(nb, 100);

	NBUartSend(nb, "AT+CGATT?\r\n");
	int8_t ret = NBUartGetResponse(nb, 100);
	if(ret == READ_COMPLETE_OK){
		char text[20];
		NBUartGetString(nb, text);
		
		NBUartSendDEBUG(nb, text);
		char *p = strstr(text,"+CGATT:");
		uint8_t len = strlen("+CGATT:");
		sprintf(text,"%s", p+len);
		NBUartSendDEBUG(nb,"\r\n");
		NBUartSendDEBUG(nb, text);

		if(*(p+len)-'0' == 0){
			return false;
		}
		return true;
	}
	return false;
}

Socket NBUartCreateSocket(NBUart* nb, uint16_t port){
	char cmd[100];
	sprintf(cmd, "AT+NSOCR=DGRAM,17,%u,1\r\n", port);
	NBUartSend(nb, cmd);
	// NBUartSendDEBUG(nb, cmd);

	Socket s; 
	int8_t ret = NBUartGetResponse(nb, 100);
	if(ret != READ_COMPLETE_OK){
		s.ID = 255;
		s.Status = 0;
		s.Port = 0;
		s.Msglen = 0;
		s.OutboundMsglen = 0;
		s.InboundMsglen = 0;
		return s;
	}

	char text[100];
	NBUartGetString(nb,text);
	// NBUartSendDEBUG(nb, "response msg: ");
	// NBUartSendDEBUG(nb, text);
	// NBUartSendDEBUG(nb, "\r\n");

	char *p = text;
	uint8_t no = *p-'0';
	s.ID = no;
	s.Status = 1;
	s.Port = port;
	s.Msglen = 0;
	s.OutboundMsglen = 0;
	s.InboundMsglen = 0;
	return s;
}

bool NBUartCloseSocket(NBUart* nb, Socket* s){
	char cmd[100];
	sprintf(cmd, "AT+NSOCL=%u\r\n", s->ID);
	NBUartSend(nb, cmd);
	// NBUartSendDEBUG(nb, cmd);

	int8_t ret = NBUartGetResponse(nb, 100);

	char text[100];
	NBUartGetString(nb,text);
	// NBUartSendDEBUG(nb, "response msg: ");
	// NBUartSendDEBUG(nb, text);
	// NBUartSendDEBUG(nb, "\r\n");
	//char* r = ResponseString(ret);
	//NBUartSendDEBUG(nb,r);
	if(ret == READ_COMPLETE_OK){
		return true;
	}
	return false;
}

bool NBUartSendPacket(NBUart* nb, Socket* s, char* destIP, uint16_t destPort, uint8_t* payload, uint16_t size){
	char cmd[100];
	sprintf(cmd, "AT+NSOST=%u,%s,%u,%d,", s->ID, destIP, destPort, size);
	NBUartSend(nb, cmd);
	// NBUartSendDEBUG(nb, cmd);

	unsigned char *p;
	uint8_t i;
	for(p = payload, i=0; i<size; p++, i++){
		sprintf(cmd, "%02X", *p);
		NBUartSend(nb, cmd);
		// NBUartSendDEBUG(nb, cmd);
	}

	sprintf(cmd, "\x0D\x0A");
	NBUartSend(nb, cmd);
	// NBUartSendDEBUG(nb, cmd);

	int8_t ret = NBUartGetResponse(nb, 100);
	if(ret == READ_COMPLETE_OK){
		return true;
	}
	return false;
}

bool NBUartFetchPacket(NBUart* nb, Socket* socket, uint16_t len, char* dst){
	// NBUartSendDEBUG(nb, "\r\npre fetch\r\n");
	NBUartGetResponse(nb, 100);
	//NBUartGetResponseT(nb, 100);

	char cmd[100];
	sprintf(cmd,"AT+NSORF=");
	NBUartSend(nb, cmd);

	// NBUartSendDEBUG(nb, "\r\n\r\n");
	// NBUartSendDEBUG(nb, cmd);

	sprintf(cmd,"%u", socket->ID);
	NBUartSend(nb, cmd);
	// NBUartSendDEBUG(nb, cmd);

	sprintf(cmd,",%u\r\n", len);
	NBUartSend(nb, cmd);
	// NBUartSendDEBUG(nb, cmd);

	// NBUartSendDEBUG(nb, "\r\n get response\r\n");

	int8_t ret;
	ret = NBUartGetResponse(nb, 200);
	if(ret == READ_COMPLETE_OK){
		//NBUartSendDEBUG(nb, "\r\nget string\r\n");
		NBUartGetString(nb, dst);
		//NBUartSendDEBUG(nb, dst);
		return true;
	}
	return false;
}

bool NBUartIsReboot(NBUart* nb){
	return nb->Reboot;
}

bool UDPInit(UDPConnection* udp, NBUart* nb){
	if(udp == NULL){
		return false;
	}

	if(nb == NULL){
		return false;
	}

	udp->NB = nb;
	sprintf(udp->DestinationIP, "NULL");
	udp->DestinationPort = 0;
	memset(udp->InboundBuffer, 0, UDP_BUFFER_SIZE*sizeof(uint8_t));
	udp->InboundBufferlen = 0;
	memset(udp->OutboundBuffer, 0, UDP_BUFFER_SIZE*sizeof(uint8_t));
	udp->OutboundBufferlen = 0;
	return true;
}

bool UDPConnect(UDPConnection* udp, char* destIP, uint16_t destPort){
	sprintf(udp->DestinationIP, "%s", destIP);
	udp->DestinationPort = destPort;

	udp->Sock = NBUartCreateSocket(udp->NB,udp->DestinationPort);
	if(!(udp->Sock).Status){
		return false;
	}
	return true;
}

bool UDPDisconnect(UDPConnection* udp){
	return NBUartCloseSocket(udp->NB, &udp->Sock);
}

bool UDPSetIP(UDPConnection* udp, char* ip){
	sprintf(udp->DestinationIP, "%s", ip);
	return true;
}

bool UDPSetPort(UDPConnection* udp, uint16_t port){
	udp->DestinationPort = port;
	return true;
}

uint16_t UDPWrite(UDPConnection* udp, const uint8_t* buffer, uint16_t size){
	if(udp->OutboundBufferlen + size <= UDP_BUFFER_SIZE){
		memcpy(udp->OutboundBuffer + udp->OutboundBufferlen, buffer, size);
		udp->OutboundBufferlen += size;
		return size;
	}else{
		return 0;
	}
}

bool UDPSendPacket(UDPConnection* udp){
	return NBUartSendPacket(udp->NB, &udp->Sock, udp->DestinationIP, udp->DestinationPort, udp->OutboundBuffer, udp->OutboundBufferlen);
}

int UDPParsePacket(UDPConnection* udp){
	uint8_t *p, *q;
	uint8_t i;	

	udp->InboundBufferlen = 0;
	memset(udp->InboundBuffer, 0, UDP_BUFFER_SIZE);
	q = udp->InboundBuffer;
	*q =  '\0';

	char inboundMsg[100];
	do{		
		// NBUartSendDEBUG(udp->NB, "*\r\n");
		bool ret;
		ret = NBUartFetchPacket(udp->NB, &udp->Sock, UART_CHUNK_SIZE, inboundMsg);
		// NBUartSendDEBUG(udp->NB, "receive: ");
		// NBUartSendDEBUG(udp->NB, inboundMsg);
		// NBUartSendDEBUG(udp->NB, "\r\n");
		if(!ret){
			break;
		}
		//char *socket;
		// char *ip;
		// char *port;
		// char *length;
		char *temp;
		char *payload;
		char *next;

		temp = strtok(inboundMsg,",");
		if(temp == NULL){
			udp->InboundBufferlen = 0;
			break;
		}
		// NBUartSendDEBUG(udp->NB, "receive: ");
		// NBUartSendDEBUG(udp->NB, socket);
		// NBUartSendDEBUG(udp->NB, "\r\n");

		temp = strtok(NULL,",");
		if(temp == NULL){
			udp->InboundBufferlen = 0;
			break;
		}
		// NBUartSendDEBUG(udp->NB, "receive: ");
		// NBUartSendDEBUG(udp->NB, ip);
		// NBUartSendDEBUG(udp->NB, "\r\n");

		temp = strtok(NULL,",");
		if(temp == NULL){
			udp->InboundBufferlen = 0;
			break;
		}
		// NBUartSendDEBUG(udp->NB, "receive: ");
		// NBUartSendDEBUG(udp->NB, port);
		// NBUartSendDEBUG(udp->NB, "\r\n");

		temp = strtok(NULL,",");
		if(temp == NULL){
			udp->InboundBufferlen = 0;
			break;
		}
		// NBUartSendDEBUG(udp->NB, "receive: ");
		// NBUartSendDEBUG(udp->NB, length);
		// NBUartSendDEBUG(udp->NB, "\r\n");

		payload = strtok(NULL,",");
		if(payload == NULL){
			udp->InboundBufferlen = 0;
			break;
		}
		// NBUartSendDEBUG(udp->NB, "receive: ");
		// NBUartSendDEBUG(udp->NB, payload);
		// NBUartSendDEBUG(udp->NB, "\r\n");

		next = strtok(NULL,",");
		if(next == NULL){
			udp->InboundBufferlen = 0;
			break;
		}
		// NBUartSendDEBUG(udp->NB, "receive: ");
		// NBUartSendDEBUG(udp->NB, next);
		// NBUartSendDEBUG(udp->NB, "\r\n");
		
		p = (uint8_t*)payload;
		while (*p != '\0') {
            *q = 0;
            for (i=0,*q=0; i<2; i++,p++) {
                *q = 16*(*q) + ((*p>='A' && *p<='F')?(*p-'A'+10):((*p>='0' && *p<='9')?(*p-'0'):0));
            }
            udp->InboundBufferlen++;
            q++;
        }

		if(strcmp(next,"0") == 0){
			break;
		}
	}while(true);
	*q = '\0';
	return udp->InboundBufferlen;
}

int UDPRead(UDPConnection* udp, uint8_t* buffer, uint16_t len){
	uint16_t rlen;
	rlen = len > udp->InboundBufferlen ? udp->InboundBufferlen : len;

	memcpy(buffer, udp->InboundBuffer, rlen);

	// termination byte
	*(buffer+rlen) = '\0';

	memmove(udp->InboundBuffer, udp->InboundBuffer+rlen, udp->InboundBufferlen-rlen);

	udp->InboundBufferlen -= rlen;

	return rlen;
}

void UDPFlush(UDPConnection* udp){
	if(udp->InboundBufferlen > 0){
		memset(udp->InboundBuffer, 0, UDP_BUFFER_SIZE);
		udp->InboundBufferlen = 0;
	}
}

void UDPFlushInbound(UDPConnection* udp){
	if(udp->InboundBufferlen > 0){
		memset(udp->InboundBuffer, 0, UDP_BUFFER_SIZE);
		udp->InboundBufferlen = 0;
	}
}

void UDPFlushOutbound(UDPConnection* udp){
	if(udp->OutboundBufferlen > 0){
		memset(udp->OutboundBuffer, 0, UDP_BUFFER_SIZE);
		udp->OutboundBufferlen = 0;
	}
}

char* UDPGetIP(UDPConnection* udp){
	return udp->DestinationIP;
}

uint16_t UDPGetPort(UDPConnection* udp){
	return udp->DestinationPort;
}

uint16_t UDPAvailable(UDPConnection* udp){
	return udp->InboundBufferlen;
}

bool UDPIsReboot(UDPConnection* udp){
	return NBUartIsReboot(udp->NB);
}