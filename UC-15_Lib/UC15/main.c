#include "stm32f10x.h"
#include "stm32f10x_conf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "CircularBuffer.h"
#include "ATparser.h"
#include "DNSPacket.h"
#include "CoAPPacket.h"
#include "UC15.h"
#include "state_machine.h"
#include "MQTTPacket.h"

char output[200];
char host[] = "broker.nexpie.io";
int port = 1883;


circular_buf_t cbuf;
atparser_t parser;
UC15 uc15;
MQTTTransport transporter;



void GPIO_init(void);
void init_usart1(void);
void init_usart2(void);
void send_byte1(uint8_t b);
void send_byte2(uint8_t b);
void usart_puts1(char *s);
void usart_puts2(char *s);

void USART1_IRQHandler(void);
void USART2_IRQHandler(void);

int readFunc(uint8_t *data);
int writeFunc(uint8_t *buffer, size_t size);
bool readableFunc(void);
void sleepFunc(int us);


static inline void Delay_1us(uint32_t nCnt_1us)
{
	volatile uint32_t nCnt;

	for (; nCnt_1us != 0; nCnt_1us--)
		for (nCnt = 4; nCnt != 0; nCnt--)
			;
}

static inline void Delay(uint32_t nCnt_1us)
{

	while (nCnt_1us--)
		;
}



void GPIO_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	//Configure LED Pin
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = 	GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = 	GPIO_Pin_0; 	
	GPIO_Init(GPIOA, &GPIO_InitStructure);
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
	// USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

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
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART2, ENABLE);
}

void USART2_IRQHandler(void)
{
	char b;
	if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET)
	{

		b = USART_ReceiveData(USART2);
		circular_buf_put(&cbuf, b);
	}
}

void send_byte1(uint8_t b)
{
	/* Send one byte */
	USART_SendData(USART1, b);

	/* Loop until USART2 DR register is empty */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
		;
}

void send_byte2(uint8_t b)
{
	/* Send one byte */
	USART_SendData(USART2, b);

	/* Loop until USART2 DR register is empty */
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
		;
}

void usart_puts1(char *s)
{
	while (*s)
	{
		send_byte1(*s);
		s++;
	}
}

void usart_puts2(char *s)
{
	while (*s)
	{
		send_byte2(*s);
		s++;
	}
}

// ATparser callback function
int readFunc(uint8_t *data)
{
	return circular_buf_get(&cbuf, data);
}

int writeFunc(uint8_t *buffer, size_t size)
{
	size_t i = 0;
	for (i = 0; i < size; i++)
	{
		send_byte2(buffer[i]);
	}
	return 0;
}

bool readableFunc()
{
	return !circular_buf_empty(&cbuf);
}

void sleepFunc(int us)
{
	Delay_1us(us);
}


void power_off(state_t* t);
void power_on(state_t* t);
void get_ready(state_t* t);
void activate(state_t* t);
void get_ip(state_t* t);
void tcp_open(state_t* t);
void tcp_disconnect(state_t* t);
void mqtt_connect(state_t* t);
void deactivate(state_t* t);
void mqtt_ping(state_t* t);
void mqtt_pub(state_t* t);


void power_off(state_t* t){
    
    usart_puts1("power off\n");
    GPIO_SetBits(GPIOA, GPIO_Pin_0);
    Delay_1us(2000000);
    GPIO_ResetBits(GPIOA, GPIO_Pin_0);
    Delay_1us(1500000);
    GPIO_SetBits(GPIOA, GPIO_Pin_0);
    Delay_1us(1000000);
    GPIO_ResetBits(GPIOA, GPIO_Pin_0);
    Delay_1us(3000000);
    //clean up
    atparser_flush(&parser);    
    next_state(t, power_on);

}

void power_on(state_t* t){
    usart_puts1("power on\n");
	GPIO_SetBits(GPIOA, GPIO_Pin_0);
	Delay_1us(2000000);
	GPIO_ResetBits(GPIOA, GPIO_Pin_0);
	Delay_1us(2000000);
	GPIO_SetBits(GPIOA, GPIO_Pin_0);
	Delay_1us(2000000);
	next_state(t, get_ready);
	
	
}

void get_ready(state_t* t){
	usart_puts1("get ready!\n");

	if(!UC15_is_ready(&uc15))
	{
		usart_puts1("not ready!\n");
		next_state(t, power_off);
	}
	else
	    next_state(t, activate);
}

void activate(state_t* t){
	usart_puts1("activate...\n");
	if(!UC15_activate(&uc15)){
		usart_puts1("activate failed!\n");
		next_state(t, power_off);
	}
    else{
    	usart_puts1("OK\n");
	    next_state(t, get_ip);
    }
}

void get_ip(state_t* t){
	usart_puts1("get ip...\n");
	
	char ip[50];
	if(!UC15_get_IP(&uc15,ip)){
		usart_puts1("get ip failed!\n");
		next_state(t, deactivate);
	}
	else{
		usart_puts1("OK\n");
		sprintf(output, "ip: %s\n",ip);
		usart_puts1(output);
		next_state(t, tcp_open);
	}
}

void tcp_open(state_t* t){

	 if(!UC15_connect(&uc15,host,port)){
        usart_puts1("tcp open error\n");
        next_state(t, deactivate);
     }
     else{
    	 usart_puts1("tcp open success\n");
	     next_state(t, mqtt_connect);
	 }
}


void tcp_disconnect(state_t* t){
 	 usart_puts1("tcp disconnect\n");
     UC15_disconnect(&uc15);
     next_state(t, tcp_open);
}



int getdata(unsigned char* buf, int count){
    int ret = UC15_read(&uc15,(char*)buf, count);
    
    int i;
    for(i=0 ; i < ret; i++ ){
    	sprintf(output,"buff = %02X \n",buf[i]);
    	usart_puts1(output);
    }
    return ret;
}

int getdata_nb(void *non ,unsigned char* buf, int count){
    return getdata(buf, count);
}

char rev_buf[400];
int rev_buflen = sizeof(rev_buf);

void mqtt_connect(state_t* t){

    //pack connect
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.clientID.cstring = "40ee821f-0a9b-4dce-94a4-d3d70b862af4";
    data.username.cstring = "cbp92g7NnWuFyY3sxXP5tPEDerjXe1WP";
    data.password.cstring = "$9gKjP7f)P4E%_%5ytXs7nMWfy0SgF&$";

    int16_t len=-1;
    char buf[400];
    int buflen = sizeof(buf);
    len = MQTTSerialize_connect((unsigned char*)buf, buflen, &data);

    usart_puts1("mqtt connect\n");
    UC15_write(&uc15, buf,len);
    
    atparser_flush(&parser);
    Delay_1us(300000);
    
    int check = 0;

    // while(!check){
    check = MQTTPacket_readnb((unsigned char*)rev_buf, rev_buflen, &transporter);
    sprintf(output,"mqtt connect check = %d \n", check);
    usart_puts1(output);
    // }

    switch(check){

        case CONNACK:
            usart_puts1("receive CONNACK\n");
            next_state(t, mqtt_pub);
            break;
        case DISCONNECT:
            usart_puts1("receive DISCONNECT\n");
            next_state(t, tcp_disconnect);
            break;
    	default :
    		next_state(t, tcp_disconnect);
            break;
    }
    
}

uint16_t temp = 0;
uint16_t humi = 0;

void mqtt_pub(state_t* t){
  
  int len=-1;
  char payload[200];
  char buf[200];
  int buflen = sizeof(buf);

  MQTTString topicString = MQTTString_initializer;
  topicString.cstring = "@shadow/data/update";

  sprintf(payload, "{\"data\" : { \"temp\": %d , \"humi\": %d } }",temp,humi);
  int payloadlen = strlen(payload);

  temp++;
  humi++;

  usart_puts1("mqtt pub\n");

  len = MQTTSerialize_publish((unsigned char*)buf, buflen, 0, 0, 0, 0, topicString, (unsigned char*)payload, payloadlen);
  UC15_write(&uc15, buf,len);    
  Delay_1us(300000);  
  
  next_state(t, mqtt_ping);
  
}




void mqtt_ping(state_t* t){
  
  int len=-1;
  unsigned char buf[10];
  int buflen = sizeof(buf);

  usart_puts1("mqtt ping\n");
  len = MQTTSerialize_pingreq(buf, buflen);
  UC15_write(&uc15, buf,len);

  atparser_flush(&parser);
  Delay_1us(300000);
  
  int check = 0;
    
    // while(!check){
    check = MQTTPacket_readnb((unsigned char*)rev_buf, rev_buflen, &transporter);
    sprintf(output,"ping check = %d \n", check);
    usart_puts1(output);
    // }

    switch(check){

        case PINGRESP:
            usart_puts1("receive PINGRESP\n");
            next_state(t, mqtt_pub);
            break;
        case DISCONNECT:
            usart_puts1("receive DISCONNECT\n");
            next_state(t, tcp_disconnect);
            break;
    	default :
    		next_state(t, tcp_disconnect);
            break;
    }



}

void deactivate(state_t* t){
	usart_puts1("deactivate...\n");
	if(!UC15_deactivate(&uc15)){
		usart_puts1("deactivate failed\n");
		next_state(t, power_off);
	}
	else{
		usart_puts1("OK\n");
		next_state(t, activate);
    }
}


//main
int main(void)
{
	state_t state = { power_off };
	/* initialize UART */
	init_usart1();
	init_usart2();
	GPIO_init();

	Delay_1us(1000000);
	usart_puts1("start\r\n");

	/* initialize circular buffer and atparser */
	usart_puts1("initialize circular buffer and at parser\n");
	circular_buf_init(&cbuf);
	atparser_init(&parser, readFunc, writeFunc, readableFunc, sleepFunc);

	transporter.sck = &uc15;
    transporter.getfn = getdata_nb;
    transporter.state = 0;	

	UC15_init(&uc15, &parser);
	Delay_1us(1000000);

  
	while (1)
	{
		usart_puts1("next>>\n");
		run_state(&state);
		Delay_1us(1000000);
	}	

}
