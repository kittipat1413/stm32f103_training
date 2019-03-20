# microgear-nb-iot-stm32
microgear library for STM32 board

ตัวอย่างการใช้งาน จะมีไฟล์ main_xxx.c

ให้เปลี่ยนชื่อไฟล์ใน Makefile เป็นไฟล์ที่ต้องการ เช่น main_microgear.c

```
#User Source Files
SRC+=	./main_microgear.c \
		./BC95.c \
		./BC95Udp.c \
		./CoAP.c \
		./Dns.c \
		./MicrogearNB.c \
		./stm32f10x_it.c 
```

## Usage detail

Declar global variables
```
NBQueue q;
NBUart nb;
UDPConnection main_udp, dns_udp;
Microgear mg;

#define APPID    "YOUR_APPID"
#define KEY      "YOUR_KEY"
#define SECRET   "YOUR_SECRET"
```
you need two udp client for DNS server and your server.

Add NBQueueInsert in IRQHandler interrupt
```
void USART1_IRQHandler(void)
{
	//char cmd[20];
    char b;    
    if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET) {

        b =  USART_ReceiveData(USART1);
        NBQueueInsert(&q, (uint8_t)b);
    }
}
```

Initialize NBQueue
```
NBQueueInit(&q);
```

Initialize NBUart
```
NBUartInit(&nb, USART1, &q);
```

Initialize UDP
```
bool ret;
ret = UDPInit(&main_udp, &nb);
```

Attach Network
```
bool ret;
do{
	ret = NBUartAttachNetwork(&nb);
	}while(!ret);
```

initialize Microgear
```
bool ret;
ret = MicrogearInit(&mg, &main_udp, &dns_udp, "coap.netpie.io", APPID, KEY, SECRET);
```

publish message
for int
```
int your_data=1;
uint16_t ret = MicrogearPublishInt(&mg, "/nbiot/data", your_data);
```
for string
```
char* your_data="hello";
uint16_t ret = MicrogearPublishString(&mg, "/nbiot/data", your_data);
```


