#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "BC95.h"
//#include <queue>
#include "BC95Udp.h"
#include "Dns.h"

void init_usart1(void);
void init_usart2(void);
void send_byte1(uint8_t b);
void send_byte2(uint8_t b);
void usart_puts1(char* s);
void usart_puts2(char* s);

void USART1_IRQHandler(void);
void USART2_IRQHandler(void);

BC95_str bc95;

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

char buffer[10];
uint8_t idx=0;
bool status = false;

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

DNS_CLIENT dns;

int main(void)
{
	init_usart1();
	init_usart2();
	BC95_init(USART1, &bc95);
	
	Delay_1us(1000000);

	char cmd[200];
	SOCKD *sock;
	char *p;

	// get IMI
	usart_puts2("get IMI..\r\n");
	p = BC95_getIMI(&bc95);
	usart_puts2(p);
	usart_puts2("\r\n");
	// sprintf(cmd,"AT+CIMI\r\n");		
	// BC95_usart_puts(cmd, bc95.USART);
	// Delay_1us(200000);	
	// p = getSerialResponse(&bc95, "\x0D\x0A");
	// usart_puts2(p);

	// attach
	usart_puts2("attach..\r\n");
	while(!BC95_attachNetwork(&bc95)){
		usart_puts2("attach..\r\n");
	}
	usart_puts2("connected..\r\n");

	// get IP
	usart_puts2("get IP..\r\n");
	char *ip;
	ip = BC95_getIPAddress(&bc95);
	if (ip == NULL) {
		usart_puts2("ip fail, exit.\r\n");
		return 0;
	}
	usart_puts2(ip);
	usart_puts2("\r\n");
	// create socket
	sock = BC95_createSocket(&bc95, 8080);
	if (sock != NULL) {
		usart_puts2("send packet..\r\n");
		char payload[] = "AB";
		uint8_t *py;
		py = (uint8_t*)payload;
		uint16_t len = strlen(payload);
		p = BC95_sendPacket(&bc95, sock, ip,8080,py,len);
		usart_puts2(p);
		usart_puts2("\r\n");
		/////////////////

		// receive packet
		Delay_1us(5000000);
		p = BC95_fetchSocketPacket(&bc95, sock, len);

		usart_puts2(p);
		usart_puts2("\r\n");

		usart_puts2("close socket\r\n");
		BC95_closeSocket(&bc95, sock);

	} else {
		usart_puts2("create socket fail, exit.\r\n");
	}

	usart_puts2("end\r\n");
}

