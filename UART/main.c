#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include <stdio.h>
#include <stdlib.h>

void init_usart1(void);
void send_byte(uint8_t b);
void usart_puts(char* s);

void USART1_IRQHandler(void);

char first_num[2];
char second_num[2];
uint8_t cnt= 0;  
uint8_t stat= 0;
char out[10];

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
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable transmit and receive interrupts for the USART1. */
	USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART1, ENABLE);

}

void USART1_IRQHandler(void)
{
    char b;
    if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET) {

          b =  USART_ReceiveData(USART1);
          
          if(stat == 0 && b == 'A'){
          	stat = 1;
          }


          else if (stat == 1){
          	
			if(b >= '0' && b <= '9'){
          		first_num[cnt++] = b;
          	}
          	else
          		stat = 0;

          	if(cnt == 2 ){
          		cnt = 0;
          		stat = 2;
        	}
          }

          else if (stat == 2){
          	if(b == '+'){
          		stat = 3;
          	}
          	else
          		stat = 0;
          }

           else if (stat == 3){

          	if(b >= '0' && b <= '9'){
          		second_num[cnt++] = b;
          	}
          	else
          		stat = 0;

          	if(cnt == 2 ){
          		cnt = 0;
          		stat = 4;
            }
		  }
          else if (stat == 4){
          	if(b == 'B'){
          		stat = 5;
          	}
          	else
          		stat = 0;
          }
          /* Uncomment this to loopback */
          // send_byte(b);
	}
}

void send_byte(uint8_t b)
{
	/* Send one byte */
	USART_SendData(USART1, b);

	/* Loop until USART2 DR register is empty */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}


void usart_puts(char* s)
{
    while(*s) {
    	send_byte(*s);
        s++;
    }
}


int main(void)
{
	// LED Pin
	GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	//Configure LED Pin
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = 	GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = 	GPIO_Pin_13; 	
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	init_usart1();
	// char b;
	



	while (1) {

			sprintf(out, "status is : %d", stat);
		    usart_puts(out);
		    usart_puts("\n");
		    Delay_1us(100000);
		    if (stat == 5){
		    	int i;
		    	char str[10];
		    	i = atoi(&first_num[0])+atoi(&second_num[0]);
  				sprintf(str, "%d", i);
		    	usart_puts(&str[0]);
		    	usart_puts("\n");
		    	usart_puts(&first_num[0]);
		    	usart_puts("\n");
		    	send_byte(second_num[0]);
		    	send_byte(second_num[1]);
		    	usart_puts("\n");
		    	stat = 0;
		    }

			// usart_puts("avilon_test\r\n");
			// usart_puts(&first_num[0]);
			// usart_puts("\n");
        	

        	// while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
        	// b =  USART_ReceiveData(USART1);
        	
        	// if(b == 97){ //97 = a
        		// GPIO_SetBits(GPIOC,GPIO_Pin_13);
        	// }
        	// else if (b == 98){ //98 = b
        		// GPIO_ResetBits(GPIOC,GPIO_Pin_13);
        	// }
        	// send_byte(b);
		
	}
}

