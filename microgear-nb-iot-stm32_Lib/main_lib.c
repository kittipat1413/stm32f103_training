#include "stm32f10x.h"
#include "stm32f10x_conf.h"
// #include "./libMicroGear/atcommand.h"
#include "./libMicroGear/NBQueue.h"
#include "./libMicroGear/NBUart.h"
#include "./libMicroGear/NBDNS.h"
#include "./libMicroGear/NBCoAP.h"
#include "./libMicroGear/NBMicrogear.h"

void init_usart1(void);
void init_usart2(void);
void send_byte1(uint8_t b);
void send_byte2(uint8_t b);
void usart_puts1(char* s);
void usart_puts2(char* s);

void USART1_IRQHandler(void);
void USART2_IRQHandler(void);

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

NBQueue q;
NBUart nb;
UDPConnection udpclient, dns_udp;
DNSClient dns;
CoAPClient ap;
Microgear mg;

#define APPID    "testSTM32iot"
#define KEY      "fpqdyJ2TSoYbjW3"
#define SECRET   "qZzPf0KjlFkYeLHsr3DbgU1ZB"

void init_usart1()
{

	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable peripheral clocks. */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* Configure USART1 Rx pin as floating input. */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure USART1 Tx as alternate function push-pull. */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure the USART1 */
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable transmit and receive interrupts for the USART1. */
	USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART1, ENABLE);
}

void init_usart2()
{

	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable peripheral clocks. */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/* Configure USART1 Rx pin as floating input. */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure USART1 Tx as alternate function push-pull. */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure the USART1 */
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable transmit and receive interrupts for the USART1. */
	USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
	//USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART2, ENABLE);
}

void USART1_IRQHandler(void)
{
	//char cmd[20];
    char b;    
    if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET) {

        b =  USART_ReceiveData(USART1);
        NBQueueInsert(&q, (uint8_t)b);

		//USART_SendData(USART2, b);
        //sprintf(cmd, "index: %u\r\n", bc95->InboundIndex);
        //usart_puts2(cmd);
    }
}


void USART2_IRQHandler(void)
{

}

void send_byte1(uint8_t b)
{
	/* Send one byte */
	USART_SendData(USART1, b);

	/* Loop until USART2 DR register is empty */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void send_byte2(uint8_t b)
{
	/* Send one byte */
	USART_SendData(USART2, b);

	/* Loop until USART2 DR register is empty */
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

void usart_puts1(char* s)
{
    while(*s) {
    	send_byte1(*s);
        s++;
    }
}

void usart_puts2(char* s)
{
    while(*s) {
    	send_byte2(*s);
        s++;
    }
}

void responseHandler(CoapPacket *packet, char* remoteIP, int remotePort){
	char buff[6];
	usart_puts2("CoAP Response Code: ");
	sprintf(buff, "%d.%02d\n", packet->Code >> 5, packet->Code & 0b00011111);
    usart_puts2(buff);

    for (int i=0; i< packet->Payloadlen; i++) {
        //Serial.print((char) (packet->payload[i]));
        char x[1];
        sprintf(x,"%c", (char) (packet->Payload[i]));
        usart_puts2(x);
    }
}

int main(void)
{
	char cmd[200];
	Delay_1us(1000000);
	init_usart1();
	init_usart2();

	usart_puts2("\r\nSTART\r\n");

	NBQueueInit(&q);
	NBUartInit(&nb, USART1, &q);
	char *out;

	out = NBUartReset(&nb);
	if(out == NULL){
		usart_puts2("NB-iot fail");
		return 0;
	}

	char *ip;
	out = NBUartGetIMI(&nb);
	usart_puts2("\r\nIMI: ");
	usart_puts2(out);

	out = NBUartGetManufacturerModel(&nb);
	usart_puts2("\r\nModel: ");
	usart_puts2(out);

	out = NBUartGetManufacturerRevision(&nb);
	usart_puts2("\r\nRevision: ");
	usart_puts2(out);

	while(1){
		bool attach = NBUartAttactNetwork(&nb);
		if(attach){
			usart_puts2("\r\nattach ok\r\n");
			break;
		} else{
			usart_puts2("\r\nattach...\r\n");
		}

		Delay_1us(500000);
	}	

	while(1){
		ip = NBUartGetIPAddress(&nb);
		usart_puts2("\r\nIP: ");
		usart_puts2(ip);

		int8_t str = NBUartGetSignalStrength(&nb);
		usart_puts2("\r\nSignal: ");
		sprintf(cmd,"%d", str);
		usart_puts2(cmd);
	}
	
	//UDP
	// bool ret;
	// ret = UDPInit(&udpclient, &nb);
	// if(!ret){
	// 	usart_puts2("udp init error\r\n");
	// }else{
	// 	usart_puts2("udp init ok\r\n");
	// }

	// ret = UDPInit(&dns_udp, &nb);
	// if(!ret){
	// 	usart_puts2("udp init error\r\n");
	// }else{
	// 	usart_puts2("udp init ok\r\n");
	// }
	// ip = NBUartGetIPAddress(&nb);
	// ret = UDPConnect(&udpclient, ip, 8080);
	// if(!ret){
	// 	usart_puts2("udp connect error\r\n");
	// }else{
	// 	usart_puts2("udp connect ok\r\n");
	// }

	// char msg[100];
	// sprintf(msg, "HelloWorld");
	// UDPWrite(&udpclient, (uint8_t*)msg, strlen(msg));
	// out = UDPSendPacket(&udpclient);
	// usart_puts2("\r\nsend udp packet\r\n");
	// usart_puts2(out);
	// Delay_1us(1000000);

	// usart_puts2("\r\nparse udp packet\r\n");
	// int len = UDPParsePacket(&udpclient);
	// usart_puts2("\r\n");
	// sprintf(cmd,"len: %d", len);
	// usart_puts2(cmd);
	// usart_puts2("\r\n");

	// usart_puts2((char*)udpclient.InboundBuffer);
	// usart_puts2("\r\n");

	// char buff[100];
	// char *pbuff;
	// pbuff = buff;
	// UDPRead(&udpclient, (uint8_t*)pbuff, len);
	// // for(uint8_t i=0; i<len+1;i++){
	// // 	sprintf(cmd,"%d %d\r\n",i,buff[i]);
	// // 	usart_puts2(cmd);
	// // }
	// usart_puts2(buff);
	// usart_puts2("\r\n");

	// ret = UDPDisconnect(&udpclient);
	// if(!ret){
	// 	usart_puts2("udp disconnect error\r\n");
	// }else{
	// 	usart_puts2("udp disconnect ok\r\n");
	// }

	// DNS test
	// ret = DNSInit(&dns, &udpclient);
	// if(!ret){
	// 	usart_puts2("dns init fail");
	// 	return 0;
	// } else {
	// 	usart_puts2("dns init ok");
	// }

	// char HostIP[20];
	// DNSSolve(&dns, "coap.netpie.io", HostIP);

	// usart_puts2("IP: ");
	// usart_puts2(HostIP);

	// DNS test end

	// CoAP test
	// usart_puts2("\r\nCoAP init...\r\n");
	// ret = CoAPInit(&ap, &udpclient, &dns_udp);
	// if(!ret){
	// 	usart_puts2("coap init fail");
	// 	return 0;
	// } else {
	// 	usart_puts2("coap init ok");
	// }

	// usart_puts2("\r\nCoAP Set Callback...\r\n");
	// ret = CoAPSetResponseCallback(&ap, responseHandler);

	// usart_puts2("\r\nCoAPStart...\r\n");
	// ret = CoAPStart(&ap);

	// usart_puts2("\r\nCoAPGet...\r\n");
	// CoAPGet(&ap, "coap.me", 5683, "/hello");

	// usart_puts2("\r\nCoAPLoop...\r\n");	
	// while(true){
	// 	CoAPLoop(&ap);
	// }
	// CoAP test end

	// Microgear test
	// usart_puts2("\r\nNBMicrogear init...\r\n");

	// ret = MicrogearInit(&mg, &udpclient, &dns_udp, "coap.netpie.io", APPID, KEY, SECRET);
	// if(!ret){
	// 	usart_puts2("\r\n Microgear init fail\r\n");
	// }

	// usart_puts2("\r\nLOOP......\r\n");
	// int cnt=0;

	// while(true){
	// 	sprintf(cmd, "%d\r\n", cnt);
	// 	usart_puts2(cmd);

	// 	uint16_t ret = MicrogearPublishInt(&mg, "/nbiot/rssi", cnt);
	// 	if(ret == 0){
	// 		usart_puts2("Microgear fail\r\n");
	// 		return 0;
	// 	}
	// 	cnt++;
	// }
	// Microgear test end

	usart_puts2("\r\nEND\r\n");
}

