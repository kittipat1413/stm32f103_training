#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include <stdio.h>
#include <stdlib.h>
#include "one_wire.h"
#include "ds18b20.h"

///////////////////////////////////
simple_float temp_data ;
one_wire_device data;
char out[40];
///////////////////////////////////
void init_usart1(void);
void delay(unsigned long ms);
void send_byte(uint8_t b);
void usart_puts(char* s);
//////////////////////////////////



void delay(unsigned long ms)
{
  volatile unsigned long i,j;
  for (i = 0; i < ms; i++ )
  for (j = 0; j < 1227; j++ );
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

    USART_Cmd(USART1, ENABLE);

}

int main(void)
{
    
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    //Configure LED Pin
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode =  GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin =   GPIO_Pin_13;    
    GPIO_Init(GPIOC, &GPIO_InitStructure);


    init_usart1();
	// Tick every 1 ms

	// ds18b20_init(GPIOA, GPIO_Pin_1, TIM2);

    while(1)
    {
        ds18b20_init(GPIOA, GPIO_Pin_1, TIM2);
        if(one_wire_reset_pulse()){
           usart_puts("ready_address");
           usart_puts("\n"); 

           data = one_wire_read_rom();
           sprintf(out, "%d:%d:%d:%d:%d:%d:%d:%d", data.address[7],data.address[6],data.address[5],data.address[4],data.address[3],data.address[2],data.address[1],data.address[0]);
           usart_puts(out);
           usart_puts("\n");
        }
        ds18b20_convert_temperature_simple();
        delay(5000);
        temp_data = ds18b20_read_temperature_simple();
        sprintf(out, "temp : %d", temp_data.integer);
        usart_puts(out);
        usart_puts("\n");
        delay(5000);


        ds18b20_init(GPIOA, GPIO_Pin_2, TIM2);
        if(one_wire_reset_pulse()){
           usart_puts("ready_address");
           usart_puts("\n"); 

           data = one_wire_read_rom();
           sprintf(out, "%d:%d:%d:%d:%d:%d:%d:%d", data.address[7],data.address[6],data.address[5],data.address[4],data.address[3],data.address[2],data.address[1],data.address[0]);
           usart_puts(out);
           usart_puts("\n");
        }
        ds18b20_convert_temperature_simple();
        delay(5000);
        temp_data = ds18b20_read_temperature_simple();
        sprintf(out, "temp : %d", temp_data.integer);
        usart_puts(out);
        usart_puts("\n");
        delay(5000);


    }
}


