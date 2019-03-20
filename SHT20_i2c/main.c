#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "SHT20.h"

typedef struct
{
	uint8_t RTC_Hours;
	uint8_t RTC_Minutes;
	uint8_t RTC_Seconds;

} RTC_DateTimeTypeDef;


void init_usart1(void);
void RTC_Init(void);
void I2C1_init(void);
void send_byte(uint8_t b);
void usart_puts(char* s);
void RTC_GetDateTime(uint32_t RTC_Counter, RTC_DateTimeTypeDef* RTC_DateTimeStruct);
uint32_t RTC_GetRTC_Counter(RTC_DateTimeTypeDef* RTC_DateTimeStruct);
void gpio_toggle(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
// bool SHT20ReadTemperature(I2C_TypeDef* i2c, uint16_t* raw);
// bool SHT20CheckCRC(uint16_t data, uint8_t crc_value);

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
    I2C_InitStructure.I2C_OwnAddress1 = 0x38;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000;

    /* I2C Peripheral Enable */
    I2C_Cmd(I2C1, ENABLE);
    /* Apply I2C configuration after enabling it */
    I2C_Init(I2C1, &I2C_InitStructure);
}


// bool SHT20ReadTemperature(I2C_TypeDef* i2c, uint16_t* raw)
// {
//   uint16_t result=0;

//   uint16_t msb=0;
//   uint16_t lsb=0;
//   uint8_t xsb=0;

//   send_byte('a');
//   I2C_AcknowledgeConfig(i2c,ENABLE);
//   I2C_GenerateSTART(i2c,ENABLE);
//   while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));
//   send_byte('b');
//   I2C_Send7bitAddress(i2c,0x80, I2C_Direction_Transmitter);
//   while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
//   send_byte('c');
//   I2C_SendData(i2c,0xE3);
//   while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
//   send_byte('d');
//   I2C_GenerateSTART(i2c,ENABLE);
//   while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));

//   I2C_Send7bitAddress(i2c, 0x80, I2C_Direction_Receiver);
//   while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

//   while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
//   msb = (uint16_t)I2C_ReceiveData(i2c);

//   while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
//   lsb = (uint16_t)I2C_ReceiveData(i2c);

//   while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
//   xsb = I2C_ReceiveData(i2c);

//   I2C_AcknowledgeConfig(i2c, DISABLE);
//   I2C_GenerateSTOP(i2c, ENABLE);
  
  
//   while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
//   I2C_ReceiveData(i2c);

//   result = (msb << 8) | lsb;

//   //check xsb
//   if(SHT20CheckCRC(result, xsb)){
//   	result &= 0xFFFC;
//   	*raw = result;
//   	return true;
//   }

//   return false;
// }

// bool SHT20CheckCRC(uint16_t data, uint8_t crc_value){
// 	uint32_t remainder = (uint32_t)data << 8;
// 	remainder |= crc_value;
// 	uint32_t divisor = (uint32_t)0x988000;
// 	int i;
// 	for(i = 0 ; i < 16 ; i++){
//         if(remainder & (uint32_t)1 << (23 - i)){
//             remainder ^= divisor;
//         }
//         divisor >>= 1;
//     }

//     if(remainder == 0){
//     	return true;
//     }
//     return false;
// }


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

	init_usart1();
    I2C1_init();
    RTC_Init();


    //Set time
    RTC_DateTimeTypeDef RTC_DateTime;
    RTC_DateTime.RTC_Hours = 17;
	RTC_DateTime.RTC_Minutes = 46;
    RTC_DateTime.RTC_Seconds = 00;
    Delay_1us(5000);
    RTC_SetCounter(RTC_GetRTC_Counter(&RTC_DateTime));
    


	while (1) {

	RTC_Counter = RTC_GetCounter();
	sprintf(buffer, "\r\n\r\nCOUNTER: %d\r\n", (int)RTC_Counter);
	usart_puts(buffer);
	RTC_GetDateTime(RTC_Counter, &RTC_DateTime);
	sprintf(buffer, "%02d:%02d:%02d\r\n",
			RTC_DateTime.RTC_Hours, RTC_DateTime.RTC_Minutes, RTC_DateTime.RTC_Seconds);
	usart_puts(buffer);


	float rawTemperature = 0;
	bool ret = SHT20ReadTemperature(I2C1, &rawTemperature);
	float tempTemperature = rawTemperature; // * (175.72 / 65536.0) -46.85;
	sprintf(buffer, "Temp: %f\r\n", tempTemperature);
	usart_puts(buffer);

	float rawHumidity = 0;
	ret = SHT20ReadHumidity(I2C1, &rawHumidity);
	float tempHumidity = rawHumidity; // * (125/ 65536.0) -6.0;
	sprintf(buffer, "Humidity: %f\r\n", tempHumidity);
	usart_puts(buffer);
	
	/* delay */
	while (RTC_Counter == RTC_GetCounter()) {}

	}

}
