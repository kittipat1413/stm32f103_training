#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "PH_OEM.h"



void init_usart1(void);
void I2C1_init(void);
void send_byte(uint8_t b);
void usart_puts(char* s);
void gpio_toggle(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);


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

void gpio_toggle(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	GPIOx->ODR ^= GPIO_Pin;
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
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // set interrupt group
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable transmit and receive interrupts for the USART1. */
	USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	// USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART1, ENABLE);

}


void I2C1_init()
{
    I2C_InitTypeDef  I2C_InitStructure;
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* Configure I2C_EE pins: SCL and SDA */
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* I2C configuration */
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x10;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000;

    /* I2C Peripheral Enable */
    I2C_Cmd(I2C1, ENABLE);
    /* Apply I2C configuration after enabling it */
    I2C_Init(I2C1, &I2C_InitStructure);
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
	/* Sawasdee OH */

	GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	//Configure LED Pin
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = 	GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = 	GPIO_Pin_13; 	
	GPIO_Init(GPIOC, &GPIO_InitStructure);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	//Configure LED Pin
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = 	GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = 	GPIO_Pin_1; 	
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//Possible output modes are:
	// GPIO_Mode_Out_OD        ;output open drain
	// GPIO_Mode_Out_PP        ;output push-pull
	// GPIO_Mode_AF_OD         ;alternate function open drain
	// GPIO_Mode_AF_PP         ;alternate function push pull
	// IMPORTANT: The first 2 are meant for main function IO pins. If Alternate function IO pin is used, the later 2 should be used. E.g. all UART transmit output pins must configure as AF output pin.
	// Refer to section 3 on a useful general IO pin configuration function.


	char buffer[80] = {'\0'};
	float rawPH = 0;
	init_usart1();
    I2C1_init();
  

	while (1) {

	OEM_ACTIVE(I2C1);
	Delay(20000);

	if(OEM_READ_PH(I2C1, &rawPH))
		{
			usart_puts("READ pH OK\r\n");
			sprintf(buffer, "PH: %d\n", (int)(rawPH*100.0f));
			usart_puts(buffer);
		}
		else
		{
			usart_puts("READ pH ERROR\r\n");
		}
	}	
	
	OEM_DEACTIVE(I2C1);
	Delay(20000);
}


