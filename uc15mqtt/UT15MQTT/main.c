#include "stm32f10x.h"
#include "stm32f10x_conf.h"

#include "UC15Queue.h"
#include "UC15Uart.h"
// #include "UC15TCP.h"
#include "UC15MQTT.h"

UC15Queue q;
UC15Uart uc;
UC15Mqtt mqtt;

void GPIO_setup(void);
void println(void);

void send_byte1(uint8_t b);
void usart_puts1(char* s);
void init_usart1(void);
void USART1_IRQHandler(void);

void send_byte2(uint8_t b);
void usart_puts2(char* s);
void init_usart2(void);
void USART2_IRQHandler(void);

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

void GPIO_setup(void)
{
	/* GPIO Sturcture */
	GPIO_InitTypeDef GPIO_InitStructure;
	/* Enable Peripheral Clock AHB for GPIOB */
	RCC_AHBPeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	/* Configure PC13 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	// GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	// GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void send_byte1(uint8_t b)
{
	/* Send one byte */
	USART_SendData(USART1, b);

	/* Loop until USART2 DR register is empty */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}


void usart_puts1(char* s)
{
    while(*s) {
    	send_byte1(*s);
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

void USART1_IRQHandler(void)
{
    // char b;
    if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET) {

          // b =  USART_ReceiveData(USART1);

          /* Uncomment this to loopback */
          // send_byte(b);
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
	if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET) {

		b =  USART_ReceiveData(USART2);
		// send_byte1(b);
		UC15QueueInsert(&q, (uint8_t)b);
	}
}

char buffer[400] = {'\0'};

int main(void)
{	
	// GPIO_setup();

	init_usart1();
	init_usart2();

	usart_puts1("hello\r\n");

	UC15QueueInit(&q);
	UC15UartInit(&uc, USART2, &q);
	// UC15TCPInit(&tcp, &uc);
	UC15MqttInit(&mqtt, &uc);

	while(!UC15UartIsReady(&uc)){
		
	}

	usart_puts1("READY!!!!!!!!!!\n");

	char *p;

	usart_puts1("GET REVISION\n");
	p = UC15UartGetRevision(&uc);
	usart_puts1(p);

	usart_puts1("GET IMI\n");
	p = UC15UartGetIMI(&uc);
	usart_puts1(p);

	while(!UC15UartInternetConnect(&uc)){

	}

	usart_puts1("Connected!!!!!!!!!!\n");

	p = UC15UartInternetGetIP(&uc);
	usart_puts1(p);

	char *host="203.151.50.158";
	char *port="8091";
	
	bool ret;

	ret = UC15MqttConnectMQTTServer(&mqtt, host, port);
	if(ret){
		usart_puts1("\nMQTT connected\n");
	}

	MqttPack connect = MqttPackCONNECT("python_test",NULL,NULL, 0, 0, 0, 0);
	ret = UC15MqttSend(&mqtt, connect.p, connect.len);
	if(ret){
		usart_puts1("\nmqtt connect send ok\n");
	}

	char *msg = "hello from 3G"; 
	MqttPack publish = MqttPackPUBLISH("test/a", (uint8_t*)msg, strlen(msg), true);
	ret = UC15MqttSend(&mqtt, publish.p, publish.len);
	if(ret){
		usart_puts1("\nmqtt pub send ok\n");
	}

	MqttPack sub = MqttPackSUBSCRIBE("echo/a");
	ret = UC15MqttSend(&mqtt, sub.p, sub.len);
	if(ret){
		usart_puts1("\nmqtt sub send ok\n");
	}


	char recv[300];
	uint16_t len=0;
	uint16_t remaining_len=0;

	while(1){
		usart_puts1("\nread>>\n");
		// bool ret = UC15UartTCPRead(&uc, '0', recv);
		
		len = UC15MqttReceive(&mqtt, (uint8_t*)recv);

		do{
			MqttPack pack = MqttUnpack((uint8_t*)recv, &len);

			usart_puts1("header: ");
			usart_puts1(pack.header);
			usart_puts1("\n");

			char out[40];
			sprintf(out, "payload len: %d\n", pack.len);
			usart_puts1(out);

			usart_puts1("payload: ");
			for(int i=0; i< pack.len; i++){
				sprintf(out, "%d,", pack.p[i]);
				usart_puts1(out);
			}
			usart_puts1("\n");

			if(len >0){
				usart_puts1("recv: ");
				for(int i=0;i<len;i++){
					send_byte1(recv[i]);
				}
			}

			sprintf(out,"remaining_len: %d\n", remaining_len);
			usart_puts1(out);

			if(len > 0){
				usart_puts1("\n do remaining_len \n");
			}

			if(strcmp(pack.header, "PUB") == 0){
				bool ret = MqttUnpackPUBLISH(&pack);
				if(ret){
					usart_puts1("topic: ");
					usart_puts1(pack.topic);
					usart_puts1("\n");
					usart_puts1("payload: ");
					for(int i=0;i<pack.payloadlength;i++){
						sprintf(out,"%c", pack.payload[i]);
						usart_puts1(out);
					}
					usart_puts1("\n");
				}
			}
		}while(len > 0);

		Delay_1us(1000000);

		MqttPack ping = MqttPackPINGREQ();
		ret = UC15MqttSend(&mqtt, ping.p, ping.len);
		if(ret){
			usart_puts1("\nmqtt ping send ok\n");
		}
	}

	usart_puts1("\nexit..\n");
	// while(1){
	// 	if(UC15UartTCPHaveComingByte(&uc)){
	// 		usart_puts1("\nflag incoming!!\n");
	// 	}
	// 	// uint8_t recv[100];
	// 	// uint16_t len=0;
	// 	// bool ret = UC15MqttReceive(&mqtt,recv,&len);
	// 	// if(ret){
	// 	// 	//print
	// 	// 	usart_puts1("\n data: ");
	// 	// 	usart_puts1((char*)recv);
	// 	// 	usart_puts1("\n");
	// 	// }else{
	// 	// 	usart_puts1("\nno incoming msg\n");
	// 	// }
		
	// 	Delay_1us(1000000);
	// 	// MqttPack ping = MqttPackPINGREQ();
	// 	// ret = UC15MqttSend(&mqtt, ping.p, ping.len);
	// 	// if(ret){
	// 	// 	usart_puts1("\nping send ok\n");
	// 	// }else{
	// 	// 	usart_puts1("\nping fail\n");
	// 	// }
	// }

	// char k[40];
	// sprintf(k, "len: %c\n", connect.len);
	// usart_puts1(k);

	// for(int i=0;i<connect.len;i++){
	// 	sprintf(k, "byte: %c %x\n", connect.p[i], connect.p[i]);
	// 	usart_puts1(k);
	// }



	ret = UC15MqttDisconnectMQTTServer(&mqtt);
	if(ret){
		usart_puts1("\nMQTT disconnected\n");
	}

	// ret = UC15UartTCPOpen(&uc, '1', '0', "TCP", host, port, "0", '0');
	// if(ret){
	// 	usart_puts1("\nTCP Opened!!!\n");
	// }else{
	// 	usart_puts1("\nTCP fail!!!\n");
	// 	return 0;
	// }

	// ret = UC15UartTCPState(&uc, '0');
	// if(ret){
	// 	usart_puts1("\nTCP OK!!!\n");
	// }else{
	// 	usart_puts1("\nTCP fail!!!\n");
	// 	return 0;
	// }

	// ret = TCPStartSend(&uc, '0');
	// if(ret){
	// 	usart_puts1("\nTCP startsend OK!!!\n");
	// }else{
	// 	usart_puts1("\nTCP startsend fail!!!\n");
	// 	return 0;
	// }

	// char *data="Hello World";
	// uint16_t datalen = strlen(data);

	// ret = TCPSend(&uc,(uint8_t*)data, datalen);
	// if(ret){
	// 	usart_puts1("\nTCP send OK!!!\n");
	// }else{
	// 	usart_puts1("\nTCP send fail!!!\n");
	// 	return 0;
	// }

	// ret = TCPStopSend(&uc);
	// if(ret){
	// 	usart_puts1("\nTCP stopsend OK!!!\n");
	// }else{
	// 	usart_puts1("\nTCP stopsend fail!!!\n");
	// 	return 0;
	// }

	// usart_puts1("\ntry read\n");

	// char recv[300];
	// while(1){
	// 	usart_puts1("\n\n\n\n\nread>>\n");
	// 	bool ret = UC15UartTCPRead(&uc, '0', recv);
	// 	usart_puts1("recv: ");
	// 	usart_puts1(recv);
	// 	Delay_1us(3000000);
	// 	if(ret){
	// 		break;
	// 	}
	// }	

	// usart_puts1("\n<<end read>>\n");
	// ret = UC15UartTCPClose(&uc, '0');
	// if(ret){
	// 	usart_puts1("\nTCP close OK!!!\n");
	// }else{
	// 	usart_puts1("\nTCP close fail!!!\n");
	// 	return 0;
	// }

	return 0;
}

void println(void){
	usart_puts1("\r\n");
}