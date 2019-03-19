#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "BC95.h"
//#include <queue>
#include "BC95Udp.h"
#include "Dns.h"
#include "CoAP.h"
#include "MicrogearNB.h"

void init_usart1(void);
void init_usart2(void);
void send_byte1(uint8_t b);
void send_byte2(uint8_t b);
void usart_puts1(char* s);
void usart_puts2(char* s);

void USART1_IRQHandler(void);
void USART2_IRQHandler(void);

BC95_str bc95;
BC95UDP udpclient;
DNS_CLIENT dns;
Coap coap;
Microgear mg;

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

// char buffer[10];
// uint8_t idx=0;
// bool status = false;

void USART1_IRQHandler(void)
{

    char b;    
    if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET) {

          b =  USART_ReceiveData(USART1);

          BC95_push(&bc95,b);
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

char remoteIP[16];
char *premoteIP;

void responseHandler(CoapPacket *packet, char* remoteIP, int remotePort){
	char buff[6];
	usart_puts2("CoAP Response Code: ");
	sprintf(buff, "%d.%02d\n", packet->code >> 5, packet->code & 0b00011111);
    usart_puts2(buff);

    for (int i=0; i< packet->payloadlen; i++) {
        //Serial.print((char) (packet->payload[i]));
        char x[1];
        sprintf(x,"%c", (char) (packet->payload[i]));
        usart_puts2(x);
    }
}

#define APPID    "testSTM32iot"
#define KEY      "fpqdyJ2TSoYbjW3"
#define SECRET   "qZzPf0KjlFkYeLHsr3DbgU1ZB"

int main(void)
{
	// init
	init_usart1();
	init_usart2();

	// bc95 init UART
	BC95_init(USART1, &bc95);
	Delay_1us(500000);
	usart_puts2("starting...\r\n");

	// attach
	while(!BC95_attachNetwork(&bc95)){
		usart_puts2("attach..\r\n");
	}
	usart_puts2("NB-IOT connected..\r\n");

	// init
	BC95Udp_init(&udpclient, &bc95);
	Microgear_udp_init(&mg, &udpclient);
	Microgear_init(&mg, APPID, KEY, SECRET);
    Microgear_begin(&mg,5555);

    char cmd[100];
    
    int cnt = 0;

	while(true){
		//usart_puts2("Sent Signal Strength: ");
		//int8_t BC95_getSignalStrength(BC95_str* bc95);
		//int8_t s = BC95_getSignalStrength(&bc95);
		sprintf(cmd,"\r\n%d\r\n", cnt);
		usart_puts2(cmd);
		Microgear_publish_int(&mg, "/nbiot/rssi", cnt);

		cnt++;
		//CoAP_loop(&coap);
		//Microgear_loop(&mg);
		//Delay_1us(500000);
	}
	// end
	usart_puts2("end\r\n");
}

