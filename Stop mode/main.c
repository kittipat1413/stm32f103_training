#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include <stdio.h>
#include <stdlib.h>


typedef struct
{
	uint8_t RTC_Hours;
	uint8_t RTC_Minutes;
	uint8_t RTC_Seconds;

} RTC_DateTimeTypeDef;


void init_Tim2(void);
void init_usart1(void);
void init_EXIT(void);
void RTC_Init(void);
void SetSysClockToHSE(void);
void send_byte(uint8_t b);
void usart_puts(char* s);
void TIM2_IRQHandler(void);
void RTC_GetDateTime(uint32_t RTC_Counter, RTC_DateTimeTypeDef* RTC_DateTimeStruct);
uint32_t RTC_GetRTC_Counter(RTC_DateTimeTypeDef* RTC_DateTimeStruct);
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

void TIM2_IRQHandler()
{
        if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET){ // clear flag (TIM_IT_Update)

                gpio_toggle(GPIOA, GPIO_Pin_1);
                gpio_toggle(GPIOC, GPIO_Pin_13);

                TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        }
}


void SetSysClockToHSE()
{
	ErrorStatus HSEStartUpStatus;
	/* SYSCLK, HCLK, PCLK2 and PCLK1 configuration -----------------------------*/
    /* RCC system reset(for debug purpose) */
    RCC_DeInit();

    /* Enable HSE */
    RCC_HSEConfig(RCC_HSE_ON);

    /* Wait till HSE is ready */
    HSEStartUpStatus = RCC_WaitForHSEStartUp();

    if (HSEStartUpStatus == SUCCESS)
    {
        /* HCLK = SYSCLK */
        RCC_HCLKConfig(RCC_SYSCLK_Div1);

        /* PCLK2 = HCLK */
        RCC_PCLK2Config(RCC_HCLK_Div1);

        /* PCLK1 = HCLK */
        RCC_PCLK1Config(RCC_HCLK_Div1);

 		/* PLLCLK = 8MHz * 9 = 72 MHz */
        RCC_PLLConfig(0x00010000, RCC_PLLMul_9);

        /* Enable PLL */
        RCC_PLLCmd( ENABLE);

        /* Wait till PLL is ready */
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
        {
        }

        /* Select PLL as system clock source */
        RCC_SYSCLKConfig( RCC_SYSCLKSource_PLLCLK);


        /* Wait till PLL is used as system clock source */
        while (RCC_GetSYSCLKSource() != 0x04)
        {
        }
    }
    else
    { /* If HSE fails to start-up, the application will have wrong clock configuration.
     User can add here some code to deal with this error */

        /* Go to infinite loop */
        while (1)
        {
        }
    }
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
	
		// NVIC_InitTypeDef NVIC_InitStructure;

		// NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;  //////////////////////////////////////////////////////////////////////////////////////////////////////
		// NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // set interrupt group
		// NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		// NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		// NVIC_Init(&NVIC_InitStructure);

		RCC_RTCCLKCmd(ENABLE);
		RTC_WaitForSynchro();

		RTC_ITConfig(RTC_IT_ALR, ENABLE);
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

	// NVIC_InitTypeDef NVIC_InitStructure;

	// NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	// NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // set interrupt group
	// NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	// NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	// NVIC_Init(&NVIC_InitStructure);

	/* Enable transmit and receive interrupts for the USART1. */
	USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	// USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART1, ENABLE);

}

void init_Tim2(){

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;

	/* Enable the TIM3 global Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // set interrupt group
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 10000-1;
	TIM_TimeBaseStructure.TIM_Prescaler = (uint16_t)(7200-1);
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	// TIM_OCInitTypeDef TIM_OCInitStruct;

	// TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	// TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
	// TIM_OCInitStruct.TIM_OutputNState = TIM_OutputNState_Disable;
	// TIM_OCInitStruct.TIM_Pulse = 50-1;
	// TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
	// TIM_OCInitStruct.TIM_OCNPolarity = TIM_OCPolarity_High;
	// TIM_OCInitStruct.TIM_OCIdleState = TIM_OCIdleState_Reset;
	// TIM_OCInitStruct.TIM_OCNIdleState = TIM_OCNIdleState_Reset;

	// TIM_OC2Init(TIM2, &TIM_OCInitStruct);
	TIM_ITConfig( TIM2, TIM_IT_Update, ENABLE); 
	TIM_Cmd(TIM2, ENABLE);

}


void init_EXIT(){

EXTI_InitTypeDef EXTI_InitStructure;

/* RTC Alarm A Interrupt Configuration */
EXTI_ClearITPendingBit(EXTI_Line17);
EXTI_InitStructure.EXTI_Line = EXTI_Line17;
EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Event;
EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
EXTI_InitStructure.EXTI_LineCmd = ENABLE;
EXTI_Init(&EXTI_InitStructure);


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

int main(void)
{
	/* Sawasdee OH */

	SetSysClockToHSE();

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
    
	// init_Tim2();
	init_usart1();
	init_EXIT();
    
    //Set time
    RTC_Init();
    RTC_DateTimeTypeDef RTC_DateTime;
    RTC_DateTime.RTC_Hours = 0;
	RTC_DateTime.RTC_Minutes = 0;
    RTC_DateTime.RTC_Seconds = 0;
    Delay_1us(5000);
    RTC_SetCounter(RTC_GetRTC_Counter(&RTC_DateTime));
    RTC_WaitForLastTask();

	while (1) {

	gpio_toggle(GPIOC, GPIO_Pin_13);
	RTC_Counter = RTC_GetCounter();
	sprintf(buffer, "\r\n\r\nCOUNTER: %d\r\n", (int)RTC_Counter);
	usart_puts(buffer);
	RTC_GetDateTime(RTC_Counter, &RTC_DateTime);
	sprintf(buffer, "%02d:%02d:%02d\r\n",
			RTC_DateTime.RTC_Hours, RTC_DateTime.RTC_Minutes, RTC_DateTime.RTC_Seconds);
	usart_puts(buffer);

	RTC_SetAlarm(RTC_GetCounter()+ 5); 
    RTC_WaitForLastTask();
	PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFE);
	/* delay */
	// while (RTC_Counter == RTC_GetCounter()) {}

	SetSysClockToHSE();

    }
}
