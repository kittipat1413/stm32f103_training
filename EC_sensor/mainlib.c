#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "CircularBuffer.h"
#include "ATparser.h"
#include "libEC/EC_rs485.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

circular_buf_t cbuf;
atparser_t parser;

void USART2_IRQHandler(void);

void init_usart1(void);
void init_usart2(void);
void NVIC_Configuration(void);

void send_byte(uint8_t b);
void send_hex(uint8_t* s,uint8_t l);
void usart_puts(char* s);

void send_byte2(uint8_t b);
void send_hex2(uint8_t* s,uint8_t l);
void usart_puts2(char* s);

void delay(unsigned long ms);


int readFunc(uint8_t *data);
int writeFunc(uint8_t *buffer, size_t size);
bool readableFunc(void);
void sleepFunc(int us);




uint8_t data_BUF[9] = {'\0'};


void USART2_IRQHandler(void)
{
    uint8_t b;

    if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET) {

        b =  USART_ReceiveData(USART2);

        circular_buf_put(&cbuf, b);

	}
}

static inline void Delay_1us(uint32_t nCnt_1us)
{
  volatile uint32_t nCnt;

  for (; nCnt_1us != 0; nCnt_1us--)
    for (nCnt = 13; nCnt != 0; nCnt--)
      ;
}

void delay(unsigned long ms)
{
  volatile unsigned long i,j;
  for (i = 0; i < ms; i++ )
  for (j = 0; j < 1000; j++ );
}

void send_byte(uint8_t b)
{
	/* Send one byte */
	USART_SendData(USART1, b);

	/* Loop until USART1 DR register is empty */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}


void usart_puts(char* s)
{
    while(*s) {
    	send_byte(*s);
        s++;
    }
}


void send_hex(uint8_t* s,uint8_t l)
{
    int i;
    for(i = 0; i<=(l-1) ; i++){
      send_byte(*s);
      s++;
    }
}

void send_byte2(uint8_t b)
{
  /* Send one byte */
  USART_SendData(USART2, b);

  /* Loop until USART2 DR register is empty */
  while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}


void usart_puts2(char* s)
{
    while(*s) {
      send_byte2(*s);
        s++;
    }
}


void send_hex2(uint8_t* s,uint8_t l)
{
    int i;
    for(i = 0; i<=(l-1) ; i++){
      send_byte2(*s);
      s++;
    }
}



void init_usart1(void)
{

	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable peripheral clocks. */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* Configure USART1 Rx pin as floating input. */
	// GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	// GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	// GPIO_Init(GPIOA, &GPIO_InitStructure);

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
	USART_InitStructure.USART_Mode = USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

	USART_Cmd(USART1, ENABLE);

}

void init_usart2(void)
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
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable transmit and receive interrupts for the USART1. */
	USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART2, ENABLE);
}


void NVIC_Configuration(void) {

    NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);

    NVIC_SetPriority(USART2_IRQn, NVIC_EncodePriority(4,1,0));

}



// ATparser callback function
int readFunc(uint8_t *data)
{
  // printf("<");
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




int main(void)
{

  init_usart1();
  init_usart2();
  NVIC_Configuration();

  usart_puts("\r\nInit OK\r\n");

  circular_buf_init(&cbuf);
  atparser_init(&parser, readFunc, writeFunc, readableFunc, sleepFunc);
  EC_rs485_init(&parser, &data_BUF[0]);


  char buffer[100];
  uint16_t Temp;
  uint16_t Humi;
  uint16_t Salt;
  uint16_t EC;
  

  while (1)
  { 
    if(EC_rs485_readTempHumi(&Temp,&Humi)){
    
      sprintf(buffer,"\nHumid: %d ",Humi);
      usart_puts(buffer);
      sprintf(buffer," Temp: %d \n",Temp);
      usart_puts(buffer);      
      }
    else{
      usart_puts("Read fail");
      atparser_set_timeout(&parser, 1000000); 
      atparser_flush(&parser);
      atparser_set_timeout(&parser, 8000000); 
    }


    if(EC_rs485_readSalt(&Salt)){

      sprintf(buffer,"\nSalt(mg/L): %d ",Salt);
      usart_puts(buffer);   
      }
    else{
      usart_puts("Read fail");
      atparser_set_timeout(&parser, 1000000); 
      atparser_flush(&parser);
      atparser_set_timeout(&parser, 8000000); 
    }


    if(EC_rs485_readEC(&EC)){
  
      sprintf(buffer,"\nEC(us/cm): %d ",EC);
      usart_puts(buffer);      
      }
    else{
      usart_puts("Read fail");
      atparser_set_timeout(&parser, 1000000); 
      atparser_flush(&parser);
      atparser_set_timeout(&parser, 8000000); 
    }
  
  }
}

