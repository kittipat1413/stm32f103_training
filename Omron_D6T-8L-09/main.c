#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "SHT20.h"



void init_usart1(void);
void send_byte(uint8_t b);
void usart_puts(char* s);

void I2C1_init(void);
void Read_DATA(I2C_TypeDef* i2c);
int D6T_checkPEC(uint8_t* buf , uint8_t pPEC );
uint8_t calc_crc(uint8_t data);

void gpio_toggle(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);


char Debug_buf[100] = {'\0'};



static inline void Delay_1us(uint32_t nCnt_1us)
{
  volatile uint32_t nCnt;

  for (; nCnt_1us != 0; nCnt_1us--)
    for (nCnt = 4; nCnt != 0; nCnt--);
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
    I2C_InitStructure.I2C_OwnAddress1 = 0x38;
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

void Read_DATA(I2C_TypeDef* i2c){
  //SET 1
  I2C_AcknowledgeConfig(i2c,ENABLE);
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));
  
  I2C_Send7bitAddress(i2c,0x14, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  
  I2C_SendData(i2c,0x02);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  I2C_SendData(i2c,0x00);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_SendData(i2c,0x01);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_SendData(i2c,0xEE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_AcknowledgeConfig(i2c, DISABLE);
  I2C_GenerateSTOP(i2c, ENABLE);

  //SET 2
  I2C_AcknowledgeConfig(i2c,ENABLE);
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));
  
  I2C_Send7bitAddress(i2c,0x14, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  
  I2C_SendData(i2c,0x05);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  I2C_SendData(i2c,0x90);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_SendData(i2c,0x3A);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_SendData(i2c,0xB8);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_AcknowledgeConfig(i2c, DISABLE);
  I2C_GenerateSTOP(i2c, ENABLE);

  //SET 3
  I2C_AcknowledgeConfig(i2c,ENABLE);
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));
  
  I2C_Send7bitAddress(i2c,0x14, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  
  I2C_SendData(i2c,0x03);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  I2C_SendData(i2c,0x00);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_SendData(i2c,0x03);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_SendData(i2c,0x8B);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_AcknowledgeConfig(i2c, DISABLE);
  I2C_GenerateSTOP(i2c, ENABLE);

  //SET 4
  I2C_AcknowledgeConfig(i2c,ENABLE);
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));
  
  I2C_Send7bitAddress(i2c,0x14, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  
  I2C_SendData(i2c,0x03);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  I2C_SendData(i2c,0x00);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_SendData(i2c,0x07);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_SendData(i2c,0x97);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_AcknowledgeConfig(i2c, DISABLE);
  I2C_GenerateSTOP(i2c, ENABLE);

  //SET 5
  I2C_AcknowledgeConfig(i2c,ENABLE);
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));
  
  I2C_Send7bitAddress(i2c,0x14, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  
  I2C_SendData(i2c,0x02);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  I2C_SendData(i2c,0x00);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_SendData(i2c,0x00);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_SendData(i2c,0xE9);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_AcknowledgeConfig(i2c, DISABLE);
  I2C_GenerateSTOP(i2c, ENABLE);


  /*This data is used to perform the Read operation to confirm that the configuration of internal registers 
  in this product have been updated. This Read operation can be skipped*/
  uint8_t data1;
  uint8_t data2;

  //SET 6
  I2C_AcknowledgeConfig(i2c,ENABLE);
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));
  
  I2C_Send7bitAddress(i2c,0x14, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  
  I2C_SendData(i2c,0x02);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));

  I2C_Send7bitAddress(i2c, 0x14, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  data1 = I2C_ReceiveData(i2c);

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  data2 = I2C_ReceiveData(i2c);

  I2C_AcknowledgeConfig(i2c, DISABLE);
  I2C_GenerateSTOP(i2c, ENABLE);

  usart_puts("Read internal registers");
  sprintf(Debug_buf, "\r\nExpected value of 2 byte read is 0x00 and 0x00 -> value is %X and %X \r\n\n", data1,data2);
  usart_puts(Debug_buf);

  //SET 7
  I2C_AcknowledgeConfig(i2c,ENABLE);
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));
  
  I2C_Send7bitAddress(i2c,0x14, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  
  I2C_SendData(i2c,0x05);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));

  I2C_Send7bitAddress(i2c, 0x14, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  data1 = I2C_ReceiveData(i2c);

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  data2 = I2C_ReceiveData(i2c);

  I2C_AcknowledgeConfig(i2c, DISABLE);
  I2C_GenerateSTOP(i2c, ENABLE);

  usart_puts("Read internal registers");
  sprintf(Debug_buf, "\r\nExpected value of 2 byte read is 0x90 and 0x3A -> value is %X and %X \r\n\n", data1,data2);
  usart_puts(Debug_buf);

  //SET 8
  I2C_AcknowledgeConfig(i2c,ENABLE);
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));
  
  I2C_Send7bitAddress(i2c,0x14, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  
  I2C_SendData(i2c,0x03);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));

  I2C_Send7bitAddress(i2c, 0x14, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  data1 = I2C_ReceiveData(i2c);

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  data2 = I2C_ReceiveData(i2c);

  I2C_AcknowledgeConfig(i2c, DISABLE);
  I2C_GenerateSTOP(i2c, ENABLE);

  usart_puts("Read internal registers");
  sprintf(Debug_buf, "\r\nExpected value of 2 byte read is 0x00 and 0x07 -> value is %X and %X \r\n\n", data1,data2);
  usart_puts(Debug_buf);

  usart_puts("\n------Delay 750 msec------\n");
  Delay_1us(1000000);


  int16_t PTAT;
  int16_t P0;
  int16_t P1;
  int16_t P2;
  int16_t P3;
  int16_t P4;
  int16_t P5;
  int16_t P6;
  int16_t P7;
  uint8_t  PEC;

  uint16_t msb;
  uint16_t lsb;

  uint8_t Data[20]; 
  uint8_t i = 0; 
  //READ Temp
  //SET 8
  I2C_AcknowledgeConfig(i2c,ENABLE);
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));
  
  I2C_Send7bitAddress(i2c,0x14, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  
  I2C_SendData(i2c,0x4C);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));

  I2C_Send7bitAddress(i2c, 0x14, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  lsb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = lsb; 
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  msb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = msb;

  PTAT = (msb << 8) | lsb;

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  lsb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = lsb;
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  msb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = msb;

  P0 = (msb << 8) | lsb;

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  lsb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = lsb;
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  msb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = msb;

  P1 = (msb << 8) | lsb;

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  lsb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = lsb;
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  msb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = msb;

  P2 = (msb << 8) | lsb;

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  lsb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = lsb;
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  msb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = msb;
  
  P3 = (msb << 8) | lsb;

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  lsb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = lsb;
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  msb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = msb;
  
  P4 = (msb << 8) | lsb;

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  lsb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = lsb;
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  msb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = msb;
  
  P5 = (msb << 8) | lsb;

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  lsb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = lsb;
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  msb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = msb;

  P6 = (msb << 8) | lsb;

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  lsb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = lsb;
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  msb = (uint16_t)I2C_ReceiveData(i2c);
  Data[i++] = msb;
  
  P7 = (msb << 8) | lsb;

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  PEC = I2C_ReceiveData(i2c);
  Data[i++] = PEC;

  I2C_AcknowledgeConfig(i2c, DISABLE);
  I2C_GenerateSTOP(i2c, ENABLE);

  D6T_checkPEC(Data , 18);

  usart_puts("Read Temperature!!!");
  sprintf(Debug_buf, "\r\nPTAT = %d C , P0 = %d C , P1 = %d C, P2 = %d C, P3 = %d C, P4 = %d C, P5 = %d C, P6 = %d C, P7 = %d C, PEC = %X \r\n\n", PTAT/10,P0/10,P1/10,P2/10,P3/10,P4/10,P5/10,P6/10,P7/10,PEC);
  usart_puts(Debug_buf);


}

uint8_t calc_crc(uint8_t data){       
	int index;       
	uint8_t temp;       
	for(index=0;index<8;index++){             
		temp = data;             
		data <<= 1;             
		if(temp & 0x80) data ^= 0x07;       
	}       
	return data;
}

int D6T_checkPEC(uint8_t* buf , uint8_t pPEC ){       
  	uint8_t crc;      
  	int i;     
 	crc = calc_crc( 0x14 );      
  	crc = calc_crc( 0x4C ^ crc );     
  	crc = calc_crc( 0x15 ^ crc );      
  	for(i=0;i<pPEC;i++){             
  		crc = calc_crc(buf[i] ^ crc );      
  	}       
  	sprintf(Debug_buf, "CRC value is %X PEC value is %X \r\n\n", crc,buf[pPEC]);
  	usart_puts(Debug_buf);
  	return (crc == buf[pPEC]);
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


	init_usart1();
    I2C1_init();

	usart_puts("START\n\n");
	
	

	while (1) {


	usart_puts("!!!!!!!LOOP!!!!!!!\n\n");
	Read_DATA(I2C1);
	Delay_1us(2000000);

	}

}
