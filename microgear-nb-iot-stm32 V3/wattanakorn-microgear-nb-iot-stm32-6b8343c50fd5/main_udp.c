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
BC95UDP udpclient;

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

// This binary string represents a UDP paylaod of the DNS query for the domain name nexpie.com
uint8_t udpdata[] = "\xC0\x5B\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00\x06\x6E\x65\x78\x70\x69\x65\x03\x63\x6F\x6D\x00\x00\x01\x00\x01";



int main(void)
{
	// init
	init_usart1();
	init_usart2();
	BC95_init(USART1, &bc95);
	Delay_1us(500000);
	usart_puts2("starting...\r\n");

	// attach
	while(!BC95_attachNetwork(&bc95)){
		usart_puts2("attach..\r\n");
	}
	usart_puts2("connected..\r\n");
	
	// get ip
	usart_puts2("get IP..\r\n");
	char *ip;
	ip = BC95_getIPAddress(&bc95);
	if (ip == NULL) {
		usart_puts2("ip fail, exit.\r\n");
		return 0;
	}
	usart_puts2(ip);
	usart_puts2("\r\n");

	// udp begin
	BC95Udp_init(&udpclient, &bc95);
	usart_puts2("udp init..\r\n");
	uint8_t begin = BC95Udp_begin(&udpclient, 8053);
	if (begin==0){
		usart_puts2(" udp init fail, exit.");
		return 0;
	} 
	usart_puts2("begin..\r\n");
	BC95Udp_beginPacket(&udpclient, "8.8.8.8", 53);
	usart_puts2("begin packet..\r\n");
	BC95Udp_write(&udpclient, udpdata, 28);
	usart_puts2("write data to udp buffer..\r\n");
	BC95Udp_endPacket(&udpclient);

	usart_puts2("\r\nsended\r\n");

	Delay_1us(5000000);

	usart_puts2("\r\nparsePacket\r\n");
	while(BC95Udp_parsePacket(&udpclient) == 0){
		usart_puts2(".");
		Delay_1us(500000);
	}

	uint8_t buff[64];
	uint8_t *pbuff;
	pbuff = buff;
	char out[100];
	int len = BC95UDP_read(&udpclient, pbuff, 64);
	usart_puts2("\n Receive UDP payload : ");
	sprintf(out,"len: %d\r\n", len);
	usart_puts2(out);

	// print
	for(int i=0;i<len;i++){
		sprintf(out,"%d %02X\r\n",i,buff[i]);
		usart_puts2(out);
	}
	
	// close
	BC95Udp_stop(&udpclient);

	usart_puts2("end\r\n");
}

