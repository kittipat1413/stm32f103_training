#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


#define SPIx_RCC      RCC_APB2Periph_SPI1
#define SPIx          SPI1
#define SPI_GPIO_RCC  RCC_APB2Periph_GPIOA
#define SPI_GPIO      GPIOA
#define SPI_PIN_MOSI  GPIO_Pin_7
#define SPI_PIN_MISO  GPIO_Pin_6
#define SPI_PIN_SCK   GPIO_Pin_5
#define SPI_PIN_SS    GPIO_Pin_4

typedef struct
{
	uint8_t RTC_Hours;
	uint8_t RTC_Minutes;
	uint8_t RTC_Seconds;

} RTC_DateTimeTypeDef;


void SPIx_Init(void);
uint8_t SPIx_Transfer(uint8_t data);
void SPIx_EnableSlave(void);
void SPIx_DisableSlave(void);
void begin(void);
void beginMeasure(void);

void init_usart1(void);
void RTC_Init(void);
void I2C1_init(void);
void send_byte(uint8_t b);
void usart_puts(char* s);
void RTC_GetDateTime(uint32_t RTC_Counter, RTC_DateTimeTypeDef* RTC_DateTimeStruct);
uint32_t RTC_GetRTC_Counter(RTC_DateTimeTypeDef* RTC_DateTimeStruct);
void gpio_toggle(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

uint8_t receivedByte;

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

void RTC_Init()
	{	
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
		PWR_BackupAccessCmd(ENABLE);
		

		// Disable RTC && Reset counter
		RCC_RTCCLKCmd(DISABLE);
		// RCC_BackupResetCmd(ENABLE); 
		// RCC_BackupResetCmd(DISABLE);

		if ((RCC->BDCR & RCC_BDCR_RTCEN) != RCC_BDCR_RTCEN) //RTCEN:RTC clock enable
		{
		
		RCC_BackupResetCmd(ENABLE); 
		RCC_BackupResetCmd(DISABLE);
		//BDRST: Backup domain software reset 
		//Set and cleared by software
		//0: Reset not activated
		//1: Resets the entire Backup domain


		RCC_LSEConfig(RCC_LSE_ON);// 32.768KHz
		while ((RCC->BDCR & RCC_BDCR_LSERDY) != RCC_BDCR_LSERDY) {} //LSERDY:External low-speed oscillator ready
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

		RTC_SetPrescaler(0x7FFF); // 1HZ set (32768/32768)

		RCC_RTCCLKCmd(ENABLE);
		RTC_WaitForSynchro();

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


void RTC_GetDateTime(uint32_t RTC_Counter, RTC_DateTimeTypeDef* RTC_DateTimeStruct) {

	unsigned long time;
	unsigned long t1;
	int hour = 0;
	int min = 0;
	int sec = 0;


	time = RTC_Counter;
	t1 = time/60;
	sec = time - t1*60;

	time = t1;
	t1 = time/60;
	min = time - t1*60;

	time = t1;
	t1 = time/24;
	hour = time - t1*24;

	
	RTC_DateTimeStruct->RTC_Hours = hour;
	RTC_DateTimeStruct->RTC_Minutes = min;
	RTC_DateTimeStruct->RTC_Seconds = sec;

}

uint32_t RTC_GetRTC_Counter(RTC_DateTimeTypeDef* RTC_DateTimeStruct) {
	
	uint32_t CNT = 0;

	CNT+=(RTC_DateTimeStruct->RTC_Hours*3600);
	CNT+=(RTC_DateTimeStruct->RTC_Minutes*60);
	CNT+=(RTC_DateTimeStruct->RTC_Seconds);

	return CNT;
}

void SPIx_Init()
{
    // Initialization struct
    SPI_InitTypeDef SPI_InitStruct;
    GPIO_InitTypeDef GPIO_InitStruct;
 
    // Step 1: Initialize SPI
    RCC_APB2PeriphClockCmd(SPIx_RCC, ENABLE);
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft | SPI_NSSInternalSoft_Set;
    SPI_Init(SPIx, &SPI_InitStruct); 
    SPI_Cmd(SPIx, ENABLE);
 
    // Step 2: Initialize GPIO
    RCC_APB2PeriphClockCmd(SPI_GPIO_RCC, ENABLE);
    // GPIO pins for MOSI, MISO, and SCK
    GPIO_InitStruct.GPIO_Pin = SPI_PIN_MOSI | SPI_PIN_MISO | SPI_PIN_SCK;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPI_GPIO, &GPIO_InitStruct);
    // GPIO pin for SS
    GPIO_InitStruct.GPIO_Pin = SPI_PIN_SS;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPI_GPIO, &GPIO_InitStruct);
 
    // Disable SPI slave device
    SPIx_DisableSlave();
}
 
uint8_t SPIx_Transfer(uint8_t data)
{

    // Wait until transmit complete
    while (!(SPIx->SR & (SPI_I2S_FLAG_TXE)));
    SPIx->DR = data;
    // Wait until receive complete
    while (!(SPIx->SR & (SPI_I2S_FLAG_RXNE)));
    // Wait until SPI is not busy anymore
    while (SPIx->SR & (SPI_I2S_FLAG_BSY));
    // Return received data from SPI data register
    return SPIx->DR;
}
 
void SPIx_EnableSlave()
{
    // Set slave SS pin low
    SPI_GPIO->BRR = SPI_PIN_SS;
}
 
void SPIx_DisableSlave()
{
    // Set slave SS pin high
    SPI_GPIO->BSRR = SPI_PIN_SS;
}

void begin() {
	    // Enable slave
        SPIx_EnableSlave();
        SPIx_Transfer(0x0A);
        Delay_1us(10);
        SPIx_Transfer(0x1F);
        Delay_1us(10);
        SPIx_Transfer(0x52);
        Delay_1us(10);
        // Disable slave
        SPIx_DisableSlave();
        Delay_1us(1000);
}

void beginMeasure() {
		char buffer[80];

		SPIx_EnableSlave();
        SPIx_Transfer(0x0B);
        Delay_1us(10);
        SPIx_Transfer(0x2D);
        Delay_1us(10);
        // dummy byte
        receivedByte = SPIx_Transfer(0);
        // Disable slave
        SPIx_DisableSlave();

        sprintf(buffer,"received 0x2D: %d\r\n", receivedByte);
        usart_puts(buffer);


        Delay_1us(1000);

        // SPIx_EnableSlave();
        // SPIx_Transfer(0x0B);
        // Delay_1us(10);
        // SPIx_Transfer(0x0B);
        // Delay_1us(10);
        // // dummy byte
        // receivedByte = SPIx_Transfer(0);        
        
        // // Disable slave
        // SPIx_DisableSlave();

        // sprintf(buffer,"received 0x0B: %d\r\n", receivedByte);
        // usart_puts(buffer);

        // turn on measurement mode
        uint8_t tempwrite = receivedByte | 0x02;

        sprintf(buffer,"tempwrite: %d\r\n", tempwrite);
        usart_puts(buffer);
        // Enable slave
        SPIx_EnableSlave();
        SPIx_Transfer(0x0A);
        Delay_1us(10);
        SPIx_Transfer(0x2D);
        Delay_1us(10);
        SPIx_Transfer(tempwrite);
        Delay_1us(10);
        // Disable slave
        SPIx_DisableSlave();
        Delay_1us(1000);


        SPIx_EnableSlave();
        SPIx_Transfer(0x0B);
        Delay_1us(10);
        SPIx_Transfer(0x02);
        Delay_1us(10);
        // dummy byte
        receivedByte = SPIx_Transfer(0);
        // Disable slave
        SPIx_DisableSlave();

        sprintf(buffer,"received 0x2D: %d\r\n", receivedByte);
        usart_puts(buffer);
        
        SPIx_EnableSlave();
        SPIx_Transfer(0x0B);
        Delay_1us(10);
        SPIx_Transfer(0x00);
        Delay_1us(10);
        // dummy byte
        receivedByte = SPIx_Transfer(0);
        // Disable slave
        SPIx_DisableSlave();

        sprintf(buffer,"received 0x00: %d\r\n", receivedByte);
        usart_puts(buffer);


        Delay_1us(1000);



        Delay_1us(1000000);
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

	uint32_t RTC_Counter = 0;
	char buffer[80] = {'\0'};
	int receivedByte ;

	init_usart1();
    SPIx_Init();
    RTC_Init();


    //Set time
    RTC_DateTimeTypeDef RTC_DateTime;
    RTC_DateTime.RTC_Hours = 17;
	RTC_DateTime.RTC_Minutes = 46;
    RTC_DateTime.RTC_Seconds = 00;
    Delay_1us(5000);
    RTC_SetCounter(RTC_GetRTC_Counter(&RTC_DateTime));

    begin() ;
    beginMeasure();

	while (1) {

	RTC_Counter = RTC_GetCounter();
	sprintf(buffer, "\r\n\r\nCOUNTER: %d\r\n", (int)RTC_Counter);
	usart_puts(buffer);
	RTC_GetDateTime(RTC_Counter, &RTC_DateTime);
	sprintf(buffer, "%02d:%02d:%02d\r\n",
			RTC_DateTime.RTC_Hours, RTC_DateTime.RTC_Minutes, RTC_DateTime.RTC_Seconds);
	usart_puts(buffer);


        SPIx_EnableSlave();
        // Write command to slave to turn on LED blinking
        SPIx_Transfer(0x0B);
        Delay_1us(10);
        // Write command to slave for asking LED blinking status
        SPIx_Transfer(0x0E);
        Delay_1us(10);
        // Read LED blinking status (off/on) from slave by transmitting 
        // dummy byte
        receivedByte = SPIx_Transfer(0);
        // Disable slave
        SPIx_DisableSlave();

    sprintf(buffer, "X : %d\r\n", receivedByte);
	usart_puts(buffer);

	
	/* delay */
	while (RTC_Counter == RTC_GetCounter()) {}

	}

}
