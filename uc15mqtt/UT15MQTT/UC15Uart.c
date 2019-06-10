#include "UC15Uart.h"

const char MSG[] = {CR,LF,CR,LF};

static inline void Delay_1us(uint32_t nCnt_1us)
{
  volatile uint32_t nCnt;

  for (; nCnt_1us != 0; nCnt_1us--)
    for (nCnt = 13; nCnt != 0; nCnt--);
}

static char *rightTrim(char *str, const char *seps)
{
    int i;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    i = strlen(str) - 1;
    while (i >= 0 && strchr(seps, str[i]) != NULL) {
        str[i] = '\0';
        i--;
    }
    return str;
}

void UC15Uart_send_byte(uint8_t b, USART_TypeDef* usartx){
	/* Send one byte */
	USART_SendData(usartx, b);

	/* Loop until USART2 DR register is empty */
	while (USART_GetFlagStatus(usartx, USART_FLAG_TXE) == RESET);
}

void UC15UartInit(UC15Uart* uc, USART_TypeDef* usartx, UC15Queue* q){
	uc->USART = usartx;
	uc->InboundQueue = q;
	memset(uc->buffer, 0, BUFFER_SIZE);

	// uc->Reboot = false;
	uc->QIURC_flag = false;
	// uc->external_topic = false;
}

void UC15UartSend(UC15Uart* uc, char* s){
	while(*s) {
		UC15Uart_send_byte(*s, uc->USART);
		s++;
	}
}

void UC15UartSendDEBUG(UC15Uart* uc, char* s){
	while(*s) {
		UC15Uart_send_byte(*s, USART1);
		s++;
	}
}

bool UC15UartIsAvailable(UC15Uart* uc){
	if( !UC15QueueIsEmpty(uc->InboundQueue) ){
		// queue has data
		return true;
	}
	return false;
}

uint8_t UC15UartReadByte(UC15Uart* uc){
	return UC15QueueRemove(uc->InboundQueue);
}

static void URCDetect(UC15Uart* uc){
	char *f = NULL;
	f = strstr(uc->buffer,"+QIURC");
	if(f != NULL){
		uc->QIURC_flag = true;
	}
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//                   UART STATUS                          //
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
bool UC15UartIsReady(UC15Uart* uc){
	//flush uart buffer
	memset(uc->buffer, 0, BUFFER_SIZE);
	char *p = uc->buffer;

	//small delay
	Delay_1us(1000);

	// char out[100];

	uint8_t i=0;
	while(UC15UartIsAvailable(uc)){
		//read byte
		*p = (char)UC15UartReadByte(uc);

		// sprintf(out, "bype: %c\n",*p);
		// UC15UartSendDEBUG(uc, out);

		if( *p == MSG[i]){
			i++;
		}

		if(i == 4){
			i = 0;
			// UC15UartSendDEBUG(uc, "\nvalid message!!\n");

			p++;
			*p = '\0';
			char *f = NULL;

			// sprintf(out, "\nstr: %s\n",uc->buffer);
			// UC15UartSendDEBUG(uc, out);

			//scan PB DONE;
			f = strstr(uc->buffer, "PB DONE");
			if(f != NULL){
				return true;
			}
		}

		//small delay
		Delay_1us(1000);
		p++;
	}
	return false;
}

char* UC15UartGetRevision(UC15Uart* uc){
	//flush uart buffer
	memset(uc->buffer, 0, BUFFER_SIZE);
	char *p = uc->buffer;
	
	//send command
	UC15UartSend(uc, "ATI\r\n");
	
	//waiting
	Delay_1us(100000);

	//receive
	// char out[100];
	uint8_t i=0;
	while(UC15UartIsAvailable(uc)){
		//read byte
		*p = (char)UC15UartReadByte(uc);

		// sprintf(out, "bype: %c\n",*p);
		// UC15UartSendDEBUG(uc, out);

		if( *p == MSG[i]){
			i++;
		}

		if(i == 4){
			i = 0;
			// UC15UartSendDEBUG(uc, "\nvalid message!!\n");

			p++;
			*p = '\0';
			char *f = NULL;

			// sprintf(out, "\nstr: %s\n",uc->buffer);
			// UC15UartSendDEBUG(uc, out);

			//scan OK;
			f = strstr(uc->buffer, "OK\r\n");
			if(f != NULL){
				f = strstr(uc->buffer, "Revision");
				if(f != NULL){
					return rightTrim(f,"\r\nOK\r\n");
				}
				return uc->buffer;
			}

			//scan ERROR;
			f = strstr(uc->buffer, "ERROR\r\n");
			if(f != NULL){
				return "ERROR";
			}

			p--;
		}

		//small delay
		Delay_1us(1000);
		p++;
	}
	return NULL;
}

char* UC15UartGetIMI(UC15Uart* uc){
	//flush uart buffer
	memset(uc->buffer, 0, BUFFER_SIZE);
	char *p = uc->buffer;
	
	//send command
	UC15UartSend(uc, "AT+GSN\r\n");
	
	//waiting
	Delay_1us(100000);

	//receive
	// char out[100];
	uint8_t i=0;
	while(UC15UartIsAvailable(uc)){
		//read byte
		*p = (char)UC15UartReadByte(uc);

		// sprintf(out, "bype: %c\n",*p);
		// UC15UartSendDEBUG(uc, out);

		if( *p == MSG[i]){
			i++;
		}

		if(i == 4){
			i = 0;
			// UC15UartSendDEBUG(uc, "\nvalid message!!\n");

			p++;
			*p = '\0';
			char *f = NULL;

			// sprintf(out, "\nstr: %s\n",uc->buffer);
			// UC15UartSendDEBUG(uc, out);

			//scan OK;
			f = strstr(uc->buffer, "OK\r\n");
			if(f != NULL){
				return rightTrim(uc->buffer,"\r\nOK\r\n");
			}

			//scan ERROR;
			f = strstr(uc->buffer, "ERROR\r\n");
			if(f != NULL){
				return "ERROR";
			}

			p--;
		}

		//small delay
		Delay_1us(1000);
		p++;
	}
	return NULL;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//                      Internet                          //
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
bool UC15UartInternetDisconnect(UC15Uart* uc){
	//flush uart buffer
	memset(uc->buffer, 0, BUFFER_SIZE);
	char *p = uc->buffer;
	
	//send command
	UC15UartSend(uc, "AT+QIDEACT=1\r\n");
	
	//waiting
	Delay_1us(1000000);

	//receive
	// char out[100];
	uint8_t i=0;
	while(UC15UartIsAvailable(uc)){
		//read byte
		*p = (char)UC15UartReadByte(uc);

		// sprintf(out, "bype: %c\n",*p);
		// UC15UartSendDEBUG(uc, out);

		if( *p == MSG[i]){
			i++;
		}

		if(i == 4){
			i = 0;
			// UC15UartSendDEBUG(uc, "\nvalid message!!\n");

			p++;
			*p = '\0';
			char *f = NULL;

			// sprintf(out, "\nstr: %s\n",uc->buffer);
			// UC15UartSendDEBUG(uc, out);

			//scan OK;
			f = strstr(uc->buffer, "OK\r\n");
			if(f != NULL){
				return true;
			}

			//scan ERROR;
			f = strstr(uc->buffer, "ERROR\r\n");
			if(f != NULL){
				return false;
			}

			p--;
		}

		//small delay
		Delay_1us(1000);
		p++;
	}
	return false;
}

bool UC15UartInternetConnect(UC15Uart* uc){
	//flush uart buffer
	memset(uc->buffer, 0, BUFFER_SIZE);
	char *p = uc->buffer;
	
	//send command
	UC15UartSend(uc, "AT+QIACT=1\r\n");
	
	//waiting
	Delay_1us(300000);

	//receive
	// char out[100];
	uint8_t i=0;
	while(UC15UartIsAvailable(uc)){
		//read byte
		*p = (char)UC15UartReadByte(uc);

		// sprintf(out, "bype: %c\n",*p);
		// UC15UartSendDEBUG(uc, out);

		if( *p == MSG[i]){
			i++;
		}

		if(i == 4){
			i = 0;
			// UC15UartSendDEBUG(uc, "\nvalid message!!\n");

			p++;
			*p = '\0';
			char *f = NULL;

			// sprintf(out, "\nstr: %s\n",uc->buffer);
			// UC15UartSendDEBUG(uc, out);

			//scan OK;
			f = strstr(uc->buffer, "OK\r\n");
			if(f != NULL){
				return true;
			}

			//scan ERROR;
			f = strstr(uc->buffer, "ERROR\r\n");
			if(f != NULL){
				return false;
			}

			p--;
		}

		//small delay
		Delay_1us(1000);
		p++;
	}
	return false;
}

char* UC15UartInternetGetIP(UC15Uart* uc){
	//flush uart buffer
	memset(uc->buffer, 0, BUFFER_SIZE);
	char *p = uc->buffer;
	
	//send command
	UC15UartSend(uc, "AT+QIACT?\r\n");
	
	//waiting
	Delay_1us(500000);

	//receive
	// char out[100];
	uint8_t i=0;
	while(UC15UartIsAvailable(uc)){
		//read byte
		*p = (char)UC15UartReadByte(uc);

		// sprintf(out, "bype: %c\n",*p);
		// UC15UartSendDEBUG(uc, out);

		if( *p == MSG[i]){
			i++;
		}

		if(i == 4){
			i = 0;
			// UC15UartSendDEBUG(uc, "\nvalid message!!\n");

			p++;
			*p = '\0';
			char *f = NULL;

			// sprintf(out, "\nstr: %s\n",uc->buffer);
			// UC15UartSendDEBUG(uc, out);

			//scan OK;
			f = strstr(uc->buffer, "+QIACT");
			if(f != NULL){
				// UC15UartSendDEBUG(uc, "\nfound QIACT\n");
				f = strstr(uc->buffer, "OK\r\n");
				if(f != NULL){
					f = strstr(uc->buffer, ","); //1
					// UC15UartSendDEBUG(uc, "\nfound QIACT OK\n");
					f = strstr(f+1, ","); //2
					f = strstr(f+1, ","); //3
					f = rightTrim(f+1,"\r\nOK\r\n");
					f = f+1;
					f = rightTrim(f, "\"");
					return f;
				}
				//return false;
			}

			//scan ERROR;
			f = strstr(uc->buffer, "ERROR\r\n");
			if(f != NULL){
				return false;
			}

			p--;
		}

		//small delay
		Delay_1us(1000);
		p++;
	}
	return NULL;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//                         TCP                            //
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
bool UC15UartTCPOpen(UC15Uart* uc, unsigned char contexid, unsigned char connectid, char* service_type, char* ip_url, char* remote_port, char* local_port, unsigned char access_mode){
	//flush uart buffer
	memset(uc->buffer, 0, BUFFER_SIZE);
	char *p = uc->buffer;

	char out[300];
	//send command
	sprintf(out, "AT+QIOPEN=%c,%c,\"%s\",\"%s\",%s,%s,%c\r\n", contexid, connectid, service_type, ip_url, remote_port, local_port, access_mode);
	// UC15UartSendDEBUG(uc, out);
	UC15UartSend(uc, out);
	
	//waiting
	Delay_1us(1000000);

	//receive
	uint8_t i=0;
	while(UC15UartIsAvailable(uc)){
		//read byte
		*p = (char)UC15UartReadByte(uc);

		// sprintf(out, "bype: %c\n",*p);
		// UC15UartSendDEBUG(uc, out);

		if( *p == MSG[i]){
			i++;
		}

		if(i == 4){
			i = 0;
			// UC15UartSendDEBUG(uc, "\nvalid message!!\n");

			p++;
			*p = '\0';
			char *f = NULL;

			// sprintf(out, "\nstr: %s\n",uc->buffer);
			// UC15UartSendDEBUG(uc, out);

			//scan OK;
			f = strstr(uc->buffer, "OK\r\n");
			if(f != NULL){
				return true;
			}

			//scan ERROR;
			f = strstr(uc->buffer, "ERROR\r\n");
			if(f != NULL){
				return false;
			}

			p--;
		}

		//small delay
		Delay_1us(1000);
		p++;
	}
	return false;
}

bool UC15UartTCPState(UC15Uart* uc, unsigned char connectid){
	//flush uart buffer
	memset(uc->buffer, 0, BUFFER_SIZE);
	char *p = uc->buffer;
	
	//send command
	char out[100];
	sprintf(out, "AT+QISTATE=1,%c\r\n", connectid);
	UC15UartSend(uc, out);
	
	//waiting
	Delay_1us(300000);

	//receive
	uint8_t i=0;
	while(UC15UartIsAvailable(uc)){
		//read byte
		*p = (char)UC15UartReadByte(uc);

		// sprintf(out, "bype: %c\n",*p);
		// UC15UartSendDEBUG(uc, out);

		if( *p == MSG[i]){
			i++;
		}

		if(i == 4){
			i = 0;
			// UC15UartSendDEBUG(uc, "\nvalid message!!\n");

			p++;
			*p = '\0';
			char *f = NULL;

			// sprintf(out, "\nstr: %s\n",uc->buffer);
			// UC15UartSendDEBUG(uc, out);

			//scan OK;
			f = strstr(uc->buffer, "+QISTATE");
			if(f != NULL){
				f = strstr(f,"OK\r\n");
				if(f != NULL){
					return true;
				}
			}

			//scan ERROR;
			f = strstr(uc->buffer, "ERROR\r\n");
			if(f != NULL){
				return false;
			}

			p--;
		}

		//small delay
		Delay_1us(1000);
		p++;
	}
	return false;
}

bool TCPStartSend(UC15Uart* uc, unsigned char connectid){
	//flush uart buffer
	memset(uc->buffer, 0, BUFFER_SIZE);
	char *p = uc->buffer;

	//send command
	char out[50];
	sprintf(out, "AT+QISEND=%c\r\n", connectid);
	UC15UartSend(uc, out);

	//waiting
	Delay_1us(1000000);
	
	//receive
	while(UC15UartIsAvailable(uc)){
		//read byte
		*p = (char)UC15UartReadByte(uc);

		// sprintf(out, "bype: %c\n",*p);
		// UC15UartSendDEBUG(uc, out);

		if( *p == '>'){
			return true;
		}
		//small delay
		Delay_1us(1000);
		p++;
	}
	return false;
}

bool TCPSend(UC15Uart* uc,uint8_t* data, uint16_t length){
	for(uint16_t i=0;i<length;i++){
		UC15Uart_send_byte(data[i], uc->USART);
	}
	return true;
}

bool TCPStopSend(UC15Uart* uc){
	//flush uart buffer
	memset(uc->buffer, 0, BUFFER_SIZE);
	char *p = uc->buffer;

	//send command
	char terminator[4];
	terminator[0] = 0x1A;
	terminator[1] = '\r';
	terminator[2] = '\n';
	terminator[3] = '\0';
	UC15UartSend(uc, terminator);

	//waiting
	Delay_1us(800000);

	//receive
	uint8_t i=0;
	while(UC15UartIsAvailable(uc)){
		//read byte
		*p = (char)UC15UartReadByte(uc);

		// sprintf(out, "bype: %c\n",*p);
		// UC15UartSendDEBUG(uc, out);

		if( *p == MSG[i]){
			i++;
		}

		if(i == 4){
			i = 0;
			// UC15UartSendDEBUG(uc, "\nvalid message!!\n");

			p++;
			*p = '\0';

			URCDetect(uc);

			char *f = NULL;

			// sprintf(out, "\nstr: %s\n",uc->buffer);
			// UC15UartSendDEBUG(uc, out);

			//scan OK;
			f = strstr(uc->buffer, "SEND OK");
			if(f != NULL){
				return true;
			}

			//scan ERROR;
			f = strstr(uc->buffer, "ERROR\r\n");
			if(f != NULL){
				return false;
			}

			p--;
		}

		//small delay
		Delay_1us(1000);
		p++;
	}
	return false;
}

bool UC15UartTCPClose(UC15Uart* uc, unsigned char connectid){
	//flush uart buffer
	memset(uc->buffer, 0, BUFFER_SIZE);
	char *p = uc->buffer;
	
	//send command
	char out[100];
	sprintf(out, "AT+QICLOSE=0\r\n");
	UC15UartSend(uc, out);
	
	//waiting
	Delay_1us(1000000);

	//receive
	uint8_t i=0;
	while(UC15UartIsAvailable(uc)){
		//read byte
		*p = (char)UC15UartReadByte(uc);

		// sprintf(out, "bype: %c\n",*p);
		// UC15UartSendDEBUG(uc, out);

		if( *p == MSG[i]){
			i++;
		}

		if(i == 4){
			i = 0;
			// UC15UartSendDEBUG(uc, "\nvalid message!!\n");

			p++;
			*p = '\0';

			URCDetect(uc);

			char *f = NULL;

			// sprintf(out, "\nstr: %s\n",uc->buffer);
			// UC15UartSendDEBUG(uc, out);

			//scan OK;
			f = strstr(uc->buffer, "OK\r\n");
			if(f != NULL){
				return true;
			}

			//scan ERROR;
			f = strstr(uc->buffer, "ERROR\r\n");
			if(f != NULL){
				return false;
			}

			p--;
		}

		//small delay
		Delay_1us(1000);
		p++;
	}
	return false;
}

bool UC15UartTCPHaveComingByte(UC15Uart* uc){
	return uc->QIURC_flag;
}

uint16_t UC15UartTCPRead(UC15Uart* uc, unsigned char connectid, char* dst){
	//flush uart buffer
	memset(uc->buffer, 0, BUFFER_SIZE);
	char *p = uc->buffer;

	//send command
	char out[100];
	sprintf(out, "AT+QIRD=%c,1500\r\n", connectid);
	UC15UartSend(uc, out);

	//waiting
	Delay_1us(1000000);

	//receive
	uint8_t i=0;

	uint16_t retry=100;
	while(retry > 0){
		if(!UC15UartIsAvailable(uc)){
			retry--;
			continue;
		}

		// retry--;
		//read byte
		*p = (char)UC15UartReadByte(uc);

		sprintf(out, "bype: %d\n",*p);
		UC15UartSendDEBUG(uc, out);

		if( *p == MSG[i]){
			i++;
		}

		if(i == 4){
			i = 0;
			retry = retry+10;
			// UC15UartSendDEBUG(uc, "\nvalid message!!\n");

			p++;
			*p = '\0';

			URCDetect(uc);

			char *f = NULL;

			// sprintf(out, "\nstr: %s\n",uc->buffer);
			// UC15UartSendDEBUG(uc, out);

			//scan OK;
			f = strstr(uc->buffer, "+QIRD: ");
			if(f != NULL){
				// UC15UartSendDEBUG(uc, "found +QIRD\n");
				//skip
				i = 2;

				retry = retry+10;
				char *h = NULL;

				// UC15UartSendDEBUG(uc, "current: ");
				// UC15UartSendDEBUG(uc, uc->buffer);
				// UC15UartSendDEBUG(uc, "\n");
				char *pp = NULL;
				pp = p-1-6;

				h = strstr(pp, "OK\r\n");
				if(h != NULL){
					// UC15UartSendDEBUG(uc, "found OK\n");
					uint16_t len = 0;
					f = f+7;
					while(*f >= '1' && *f <= '9'){
						len = len*10+(*f - '0');
						// sprintf(out, "\n new byte len: %d\n", len);
					// 	UC15UartSendDEBUG(uc, out);
						f++;
					}
					f++;//CR
					f++;//LF
					
					// sprintf(out, "\nstr OK: %s\n",f);
					// UC15UartSendDEBUG(uc, out);

					sprintf(out, "\n new byte len: %d\n", len);
					UC15UartSendDEBUG(uc, out);
					
					memcpy(dst, f, len);

					if(len <= 0){
						return 0;
					}

					f = dst;
					f = f+len;
					*f = '\0';

					uc->QIURC_flag = false;
					return len;
				}				
			}

			//scan ERROR;
			f = strstr(uc->buffer, "ERROR\r\n");
			if(f != NULL){
				return 0;
			}

			p--;
		}

		//small delay
		Delay_1us(1000);
		p++;
		retry--;
	}
	return 0;
}