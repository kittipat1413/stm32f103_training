#include "BC95.h"

////////////////////////////////////////////////////////
// define
////////////////////////////////////////////////////////
#define READ_OK      1
#define READ_TIMEOUT      -1
#define READ_OVERFLOW     -2
#define READ_INCOMPLETE   -3
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define BUFFERSIZE DATA_BUFFER_SIZE
const char BC95_STOPPER[][STOPPERLEN] = {END_LINE, END_OK, END_ERROR};
#define STOPPERCOUNT COUNT_OF(BC95_STOPPER)
char BC95_buffer[BC95_BUFFER_SIZE];
uint8_t BC95_buffer_idx=0;
uint8_t BC95_idx=0;
int BC95_h[STOPPERCOUNT] = {0};
int BC95_buffer_status = READ_INCOMPLETE;



////////////////////////////////////////////////////////
// utility
////////////////////////////////////////////////////////
void BC95_send_byte(uint8_t b, USART_TypeDef* usartx);
void BC95_usart_puts(char* s, USART_TypeDef* usartx);
int readUntilDone(BC95_str* bc95, char* buff, uint32_t timeout, size_t maxsize);
char* getSerialResponse(BC95_str* bc95, char *prefix);

static inline void Delay_1us(uint32_t nCnt_1us)
{
  volatile uint32_t nCnt;

  for (; nCnt_1us != 0; nCnt_1us--)
    for (nCnt = 13; nCnt != 0; nCnt--);
}

static inline void Delay(uint32_t nCnt_1us)
{

			while(nCnt_1us--);
}

void BC95_send_byte(uint8_t b, USART_TypeDef* usartx)
{
	/* Send one byte */
	USART_SendData(usartx, b);

	/* Loop until USART2 DR register is empty */
	while (USART_GetFlagStatus(usartx, USART_FLAG_TXE) == RESET);
}

void BC95_usart_puts(char* s, USART_TypeDef* usartx)
{
    while(*s) {
    	BC95_send_byte(*s, usartx);
        s++;
    }
}

char* parseRespose(char *rawstr,char* pref){
	uint8_t i;
	char *r, *p;

    if (pref==NULL || *pref=='\0') {
        return rawstr;
    }
    p = strstr((const char *)rawstr, (const char *)pref);
	if (p != NULL) {
		p += strlen(pref);
        // skip STOPPER[0]

        for (i=1; i<STOPPERCOUNT; i++) {
            r = strstr((const char *)p, (const char *)BC95_STOPPER[i]);
            if (r != NULL) {
                *r = '\0';
    	    	r += strlen(BC95_STOPPER[i]);
                return p;
            }
        }
	}
	return NULL;
}

char* getSerialResponse(BC95_str* bc95, char *prefix) {
	if( BC95_buffer_status >= 0){
		char *p, *r;
		uint8_t sno;
		size_t plen;

		p = strstr((const char *)BC95_buffer, "+NSONMI:");
		if (p != NULL) {
			// DEBUG
			// char cmd[100] = "(not NULL)Incoming data : ";
			// BC95_usart_puts(cmd, USART2);
			// sprintf(cmd,"buffer: %s\r\n", BC95_buffer);
			// BC95_usart_puts(cmd, USART2);

			// sprintf(cmd,"skip NSONMI\r\n");
			// BC95_usart_puts(cmd, USART2);
			p = p+8; // skip +NSONMI:		

			plen = 0;

			// sprintf(cmd,"set plen = 0\r\n");
			// BC95_usart_puts(cmd, USART2);
			for(r=p+2; *r>='0' && *r<='9'; r++){
				// sprintf(cmd,"cal plen\r\n");
				// BC95_usart_puts(cmd, USART2);
				plen = 10*plen + *r - '0';				
			}

			// sprintf(cmd,"cal sno\r\n");
			// BC95_usart_puts(cmd, USART2);
			sno = (unsigned char)*p - '0';

			// sprintf(cmd,"check max socket\r\n");
			// BC95_usart_puts(cmd, USART2);
			if (sno < MAXSOCKET) {
				//char cmd[20];
				// sprintf(cmd,"set plen\r\n");
				// BC95_usart_puts(cmd, USART2);

				bc95->socketpool[sno].bc95_msglen = plen;
				// sprintf(cmd,"msg plen\r\n");
				// BC95_usart_puts(cmd, USART2);
				// sprintf(cmd,"%d\r\n", plen);
				// BC95_usart_puts(cmd, USART2);
			}

			Delay_1us(200000);
			if( BC95_buffer_status >=0){
				BC95_buffer_idx=0;	
				return parseRespose(BC95_buffer,prefix);
			} else {
				BC95_buffer_idx=0;	
				return NULL;
			}					
		}
		else {
			// DEBUG
			// char cmd[] = "Incoming data : ";
			// BC95_usart_puts(cmd, USART2);
			// sprintf(cmd,"%s\r\n", BC95_buffer);
			// BC95_usart_puts(cmd, USART2);

			BC95_buffer_idx=0;
			return parseRespose(BC95_buffer,prefix);
		}
	}
	else {
		BC95_buffer_idx=0;
		return NULL;
	}
}

int readUntilDone(BC95_str* bc95, char* buff, uint32_t timeout, size_t maxsize){

	return 0;
}


////////////////////////////////////////////////////////
// function
////////////////////////////////////////////////////////



void BC95_init(USART_TypeDef* usartx, BC95_str* bc95){
	bc95->USART = usartx;
}

char* BC95_fetchSocketPacket(BC95_str* bc95, SOCKD* socket, uint16_t len)
{
	char *p;
    if((p = getSerialResponse(bc95,"\x0D\x0A"))) return p;

    char cmd[100];

    sprintf(cmd,"AT+NSORF=");
	BC95_usart_puts(cmd, bc95->USART);

    sprintf(cmd,"%u",socket->sockid);
	BC95_usart_puts(cmd, bc95->USART);

    sprintf(cmd,",%u\r\n",len);
	BC95_usart_puts(cmd, bc95->USART);

	Delay_1us(1000000);
	
    p = getSerialResponse(bc95,"\x0D\x0A");
    return p;
}

void BC95_reset(BC95_str* bc95){
	char cmd[100];
	sprintf(cmd,"AT+NRB");
	BC95_usart_puts(cmd, bc95->USART);
	Delay_1us(2000000);
	getSerialResponse(bc95,"REBOOTING");
	sprintf(cmd,"AT+CFUN=1");
	BC95_usart_puts(cmd, bc95->USART);
	Delay_1us(2000000);
	sprintf(cmd,"AT");
	BC95_usart_puts(cmd, bc95->USART);
	Delay_1us(2000000);
}

bool BC95_attachNetwork(BC95_str* bc95){
	BC95_usart_puts("AT+CGATT=1\r\n", bc95->USART);
	BC95_usart_puts("AT+CGATT?\r\n", bc95->USART);

	Delay_1us(1000000);
	// TODO receive response
	return getSerialResponse(bc95, "+CGATT:1");
}

char* BC95_getIMI(BC95_str* bc95){
	char cmd[100];
	sprintf(cmd,"AT+CIMI\r\n"); 
	BC95_usart_puts(cmd, bc95->USART);

	Delay_1us(50000);

	// TODO receive response	
	return getSerialResponse(bc95, "\x0D\x0A");
}

char* BC95_getManufacturerModel(BC95_str* bc95){
	BC95_usart_puts("AT+CGMM\r\n", bc95->USART);

	Delay_1us(200000);
	// TODO receive response

	return getSerialResponse(bc95, "\x0D\x0A");
}

char* BC95_getManufacturerRevision(BC95_str* bc95){
	BC95_usart_puts("AT+CGMR\r\n", bc95->USART);

	Delay_1us(200000);
	// TODO receive response
	
	return getSerialResponse(bc95, "\x0D\x0A");
}

char* BC95_getIPAddress(BC95_str* bc95){
	BC95_usart_puts("AT+CGPADDR=0\r\n", bc95->USART);
	
	Delay_1us(200000);
	// TODO receive response
	
	return getSerialResponse(bc95, "+CGPADDR:0,");
}

int8_t BC95_getSignalStrength(BC95_str* bc95){
	BC95_usart_puts("AT+CSQ\r\n", bc95->USART);
	
	Delay_1us(200000);
	// TODO receive response
	char *p;
	p = getSerialResponse(bc95, "+CSQ:");
	int8_t r = 0, i;
	for (i=0; i<2; i++) {
        if (*p>='0' && *p<='9'){
        	r=10*r + *p-'0';
        }
        p++;
    }

	return 2*r-113;
}

SOCKD* BC95_createSocket(BC95_str* bc95, uint16_t port){
	uint8_t no;
	char cmd[100];
	// TODO create socket 
	if(bc95 == NULL){
		sprintf(cmd,"NULL bc95");
		BC95_usart_puts(cmd, USART2);
		return NULL;
	}
	sprintf(cmd,"Create socket..");
	BC95_usart_puts(cmd, USART2);
	
	sprintf(cmd, "AT+NSOCR=DGRAM,17,%d,1\r\n",port);
	BC95_usart_puts(cmd, bc95->USART);
	BC95_usart_puts(cmd, USART2);

	Delay_1us(5000000);
	no = *(getSerialResponse(bc95,"\x0D\x0A")) - '0';
	if (no<MAXSOCKET) {
        bc95->socketpool[no].sockid = no;
        bc95->socketpool[no].status = 1;
        bc95->socketpool[no].port = port;

        sprintf(cmd,"OK");
        BC95_usart_puts(cmd, USART2);
        return &(bc95->socketpool[no]);
    }
    else {
    	sprintf(cmd,"FAIL");
        BC95_usart_puts(cmd, USART2);
        return NULL;
    }
}

char* BC95_sendPacket(BC95_str* bc95, SOCKD* socket, char* destIP, uint16_t destPort, uint8_t* payload, uint16_t size) {
	uint8_t i;
	unsigned char *p;
	char cmd[100];

	//sprintf(cmd,"\r\nbc95 run sendPacket\r\n");
	//BC95_usart_puts(cmd, USART2);

	sprintf(cmd,"AT+NSOST=%u,%s,%u,%d,", socket->sockid,destIP,destPort,size);
	BC95_usart_puts(cmd, bc95->USART);
	//BC95_usart_puts(cmd, USART2);

	for(p=payload, i=0; i<size; p++,i++){
		sprintf(cmd,"%02X",*p); 
		BC95_usart_puts(cmd, bc95->USART);
		//BC95_usart_puts(cmd, USART2);
	}
	// CRNL
	sprintf(cmd,"\x0D\x0A");
	BC95_usart_puts(cmd, bc95->USART);
	//BC95_usart_puts(cmd, USART2);

	//sprintf(cmd,"\r\nbc95 sended\r\n");
	//BC95_usart_puts(cmd, USART2);

	Delay_1us(5000);
	getSerialResponse(bc95,"\x0D\x0A");
	char k[1];
	char *pk;
	sprintf(k,"1");
	pk = k;
	return pk;
}

char* BC95_closeSocket(BC95_str* bc95, SOCKD* socket) {
	char cmd[100];
	sprintf(cmd,"AT+NSOCL=");
	BC95_usart_puts(cmd, bc95->USART);
	sprintf(cmd,"%u\r\n", socket->sockid);
	BC95_usart_puts(cmd, bc95->USART);

	char *p;
	p = getSerialResponse(bc95,"");
	bc95->socketpool[socket->sockid].status = 0;
	return p;
}

void BC95_push(BC95_str* bc95,char b) {
	//get pointer
	char *p = &BC95_buffer[BC95_buffer_idx];
	
	//read
	*p = b;
	BC95_buffer_idx++;	
	//USART_SendData(USART2, '*');

	uint8_t i;
	for(i=0;i<STOPPERCOUNT;i++){
		if (BC95_STOPPER[i][BC95_h[i]] != *p){
			BC95_h[i] = 0;
		} else {
			BC95_h[i]++;
		}

		if(BC95_STOPPER[i][BC95_h[i]] == '\0'){
			if (i > 0 || (i==0 && strstr(BC95_buffer, "+NSONMI:")!=NULL)) {
				*(p+1) = '\0';
				BC95_buffer_status = i;
				//USART_SendData(USART2, i+'0');
				//while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
				return;
			}
		}
		if (p - BC95_buffer > BUFFERSIZE) {
			*(p+1) = '\0';
			BC95_buffer_status = READ_OVERFLOW;
			//USART_SendData(USART2, 'Z');
			//while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
			return;
		}
		if (*p == '\0') {
			BC95_buffer_status = READ_INCOMPLETE;
			//USART_SendData(USART2, 'X');
			//while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
			return;
		}
	}	
}

// 10.160.2.126
// create socket  AT+NSOCR=DGRAM,17,8080,1
// send AT+NSOST=0,10.160.2.126,8080,2,AB30
// receive AT+NSORF=0,2

