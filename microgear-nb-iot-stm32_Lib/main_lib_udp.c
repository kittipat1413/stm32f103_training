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
UDPConnection main_udp, dns_udp;
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
	char text[200];
	bool ret= false;
	uint16_t datalen=0;
	// small delay
	Delay_1us(1000000);

	// initialize UART
	init_usart1();
	init_usart2();

	usart_puts2("START\r\n");

	// initialize NB-iot Queue
	NBQueueInit(&q);
	// initialize NB-iot UART
	NBUartInit(&nb, USART1, &q);
	// initialize NB-iot UDP
	ret = UDPInit(&main_udp, &nb);
	if(!ret){
		usart_puts2("UDP init error.\r\n");
		return 0;
	}

	//attach to the network
	do{
		usart_puts2("attact...\r\n");
		ret = NBUartAttachNetwork(&nb);
	}while(!ret);

	// get IMI
	usart_puts2("device IMI: ");
	ret = NBUartGetIMI(&nb, text);
	if(ret){
		usart_puts2(text);
		usart_puts2("\r\n");
	}

	usart_puts2("device IP: ");
	char ip[20];
	ret = NBUartGetIPAddress(&nb, ip);
	if(ret){
		usart_puts2(ip);
		usart_puts2("\r\n");
	}

	// UDP Connect
	ret = UDPConnect(&main_udp, ip, 8080);
	if(!ret){
		usart_puts2("UDP Connect error.\r\n");
		return 0;
	}

	// send UDP packet
	sprintf(text, "HelloWorld");
	datalen = UDPWrite(&main_udp, (uint8_t*)text, strlen(text));
	ret = UDPSendPacket(&main_udp);
	Delay_1us(3000000);

	// receive UDP packet
	int inboundDataLen=0;
	inboundDataLen = UDPParsePacket(&main_udp);
	sprintf(text,"inbound len:%d\r\n", inboundDataLen);
	usart_puts2(text);

	// handle UDP packet
	UDPRead(&main_udp, (uint8_t*)text, inboundDataLen);
	usart_puts2(text);
	usart_puts2("\r\n");
	
	// UDP Disconnect
	ret = UDPDisconnect(&main_udp);
	if(!ret){
		usart_puts2("UDP Disconnect error.\r\n");
		return 0;
	}
	usart_puts2("END\r\n");
}

