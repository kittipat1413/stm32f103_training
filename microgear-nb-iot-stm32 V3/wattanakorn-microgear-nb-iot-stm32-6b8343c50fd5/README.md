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


