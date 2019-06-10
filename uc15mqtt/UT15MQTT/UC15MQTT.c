#include "UC15MQTT.h"

static inline void Delay_1us(uint32_t nCnt_1us)
{
  volatile uint32_t nCnt;

  for (; nCnt_1us != 0; nCnt_1us--)
    for (nCnt = 13; nCnt != 0; nCnt--);
}

static uint16_t writeString(char* string, uint8_t* buf, uint16_t pos){
	const char* idp = string;
	uint16_t i = 0;
	pos += 2;
	while (*idp) {
		buf[pos++] = *idp++;
		i++;
	}
	buf[pos-i-2] = (i >> 8);
	buf[pos-i-1] = (i & 0xFF);
	return pos;
}

bool UC15MqttInit(UC15Mqtt* mq, UC15Uart* uc){
	if(uc == NULL){
		return false;
	}

	// mq->Callback = func_null;
	mq->connected = false;
	mq->uc = uc;
	return true;
}

bool UC15MqttConnectMQTTServer(UC15Mqtt* mq, char* host, char* port){
	if(mq->uc == NULL){
		return false;
	}
	// ret = UC15TCPOpen(&tcp,'1','0',"TCP", "203.151.50.158", "1883", "0", '0');
	mq->connected = UC15UartTCPOpen(mq->uc, '1', '0', "TCP", host, port, "0", '0');
	return mq->connected;
}

bool UC15MqttDisconnectMQTTServer(UC15Mqtt* mq){
	mq->connected = false;
	return UC15UartTCPClose(mq->uc, '0');
}

bool UC15MqttSend(UC15Mqtt* mq, uint8_t* pkg, uint16_t pkglen){
	bool ret;

	ret = TCPStartSend(mq->uc, '0');
	if(!ret){
		return ret;
	}

	ret = TCPSend(mq->uc, pkg, pkglen);
	if(!ret){
		ret = TCPStopSend(mq->uc);
		return ret;
	}

	ret = TCPStopSend(mq->uc);
	return ret;
}

uint16_t UC15MqttReceive(UC15Mqtt* mq, uint8_t* dst){
	return UC15UartTCPRead(mq->uc, '0', (char*)dst);
}

MqttPack MqttUnpack(uint8_t* src, uint16_t* len){
	MqttPack pack;

	if(*len <= 0){
		pack.p[0] = '\0';
		pack.len = 0;
		return pack;
	}

	uint16_t i=0;
	switch(src[0]){
		case 0x20:
				sprintf(pack.header,"CONNACK");
				for(uint16_t j=0;j<src[1];j++,i++){
					pack.p[i] = src[2+j];
				}
				pack.p[i] = '\0';
				pack.len = src[1];
				*len = *len - src[1] -2;
				memmove(src, src+2+pack.len, *len);
			break;
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x34:
				sprintf(pack.header,"PUB");
				for(uint16_t j=0;j<src[1];j++,i++){
					pack.p[i] = src[2+j];
				}
				pack.p[i] = '\0';
				pack.len = src[1];
				*len = *len - src[1] -2;
				memmove(src, src+2+pack.len, *len);
			break;
		case 0x90:
				sprintf(pack.header,"SUBACK");
				for(uint16_t j=0;j<src[1];j++,i++){
					pack.p[i] = src[2+j];
				}
				pack.p[i] = '\0';
				pack.len = src[1];
				*len = *len - src[1] -2;
				memmove(src, src+2+pack.len, *len);
			break;
		case 0xD0:
				sprintf(pack.header,"PING");
				for(uint16_t j=0;j<src[1];j++,i++){
					pack.p[i] = src[2+j];
				}
				pack.p[i] = '\0';
				pack.len = src[1];
				*len = *len - src[1] -2;
				memmove(src, src+2+pack.len, *len);
			break;
	}
	return pack;
}

bool MqttUnpackPUBLISH(MqttPack* pack){
	if( strcmp(pack->header, "PUB") == 0){
		uint16_t topic_len = ((uint16_t)pack->p[0] << 8) | (uint16_t)pack->p[1];
		for(int i=0;i<topic_len;i++){
			pack->topic[i] = pack->p[2+i];
		}
		pack->topic[topic_len] = '\0';

		pack->payloadlength = pack->len-2-topic_len;

		for(int i=0; i< pack->len-2-topic_len; i++){
			pack->payload[i] = pack->p[topic_len+2+i];
		}
		return true;
	}
	return false;
}


MqttPack MqttPackCONNECT(char* clientID, char* username, char* password, char* willTopic, uint8_t willQos, bool willRetain, char* willMessage){
	uint16_t length = 5;
	unsigned int j;

	static uint8_t buffer[MQTT_MAX_PACKET_SIZE];

	uint8_t d[7] = {0x00,0x04,'M','Q','T','T',MQTT_VERSION};
#define MQTT_HEADER_VERSION_LENGTH 7

	//byte 5-11
	for (j = 0;j<MQTT_HEADER_VERSION_LENGTH;j++) {
		buffer[length++] = d[j];
	}

	uint8_t v;
	if (willTopic) {
		v = 0x06|(willQos<<3)|(willRetain<<5);
	} else {
		v = 0x02;
	}

	if(username != NULL) {
		v = v|0x80;

		if(password != NULL) {
			v = v|(0x80>>1);
		}
	}

	buffer[length++] = v;
	buffer[length++] = ((MQTT_KEEPALIVE) >> 8);
	buffer[length++] = ((MQTT_KEEPALIVE) & 0xFF);



	length = writeString(clientID,buffer,length);
	if (willTopic) {
		length = writeString(willTopic, buffer,length);
		length = writeString(willMessage, buffer,length);
	}

	if(username != NULL) {
		length = writeString(username, buffer,length);
		if(password != NULL) {
			length = writeString(password, buffer,length);
		}
	}

	// for(int i=5;i<length;i++){
	// 	UC15Uart_send_byte(buffer[i], USART1);
	// 	UC15Uart_send_byte('\n', USART1);
	// }


	uint8_t lenBuf[4];
	uint8_t llen = 0;
	uint8_t digit;
	uint8_t pos = 0;
	uint16_t len = length-5;
	do {
		digit = len % 128;
		len = len / 128;
		if (len > 0) {
			digit |= 0x80;
		}
		lenBuf[pos++] = digit;
		llen++;
	} while(len>0);

	uint8_t header = MQTTCONNECT;
	buffer[4-llen] = header;
	for (int i=0;i<llen;i++) {
		buffer[5-llen+i] = lenBuf[i];
	}

	MqttPack pack;
	pack.len = length-5+1+llen;
	memcpy(pack.p, buffer+(4-llen), pack.len);

	for(int i=0;i<pack.len;i++){
		UC15Uart_send_byte(pack.p[i], USART1);
		UC15Uart_send_byte('\n', USART1);
	}

	return pack;
}

MqttPack MqttPackPUBLISH(char* topic, uint8_t* payload, uint16_t payloadlength, bool retained){
	uint16_t length = 5;
	// unsigned int j;

	static uint8_t buffer[MQTT_MAX_PACKET_SIZE];

	//data
	length = writeString(topic, buffer, length);
	uint16_t i;
	for (i=0;i<payloadlength;i++) {
		buffer[length++] = payload[i];
	}
	uint8_t header = MQTTPUBLISH;
	if (retained) {
		header |= 1;
	}

	// for(int i=5;i<length;i++){
	// 	UC15Uart_send_byte(buffer[i], USART1);
	// 	UC15Uart_send_byte('\n', USART1);
	// }


	// calculate length
	uint8_t lenBuf[4];
	uint8_t llen = 0;
	uint8_t digit;
	uint8_t pos = 0;
	uint16_t len = length-5;
	do {
		digit = len % 128;
		len = len / 128;
		if (len > 0) {
			digit |= 0x80;
		}
		lenBuf[pos++] = digit;
		llen++;
	} while(len>0);

	buffer[4-llen] = header;
	for (int i=0;i<llen;i++) {
		buffer[5-llen+i] = lenBuf[i];
	}

	//packing
	MqttPack pack;
	pack.len = length-5+1+llen;
	memcpy(pack.p, buffer+(4-llen), pack.len);

	for(int i=0;i<pack.len;i++){
		UC15Uart_send_byte(pack.p[i], USART1);
		UC15Uart_send_byte('\n', USART1);
	}

	return pack;
}

MqttPack MqttPackPINGREQ(void){
	MqttPack pack;

	pack.p[0] = 0xC0;
	pack.p[1] = 0x00;
	pack.len = 2;
	return pack;
}

MqttPack MqttPackSUBSCRIBE(char* topic){
	uint8_t qos=0;
	uint16_t length = 5;

	static uint8_t buffer[MQTT_MAX_PACKET_SIZE];

	uint8_t nextMsgId=1;
	buffer[length++] = (nextMsgId >> 8);
	buffer[length++] = (nextMsgId & 0xFF);

	length = writeString(topic, buffer, length);
	buffer[length++] = qos;

	// calculate length
	uint8_t lenBuf[4];
	uint8_t llen = 0;
	uint8_t digit;
	uint8_t pos = 0;
	uint16_t len = length-5;
	do {
		digit = len % 128;
		len = len / 128;
		if (len > 0) {
			digit |= 0x80;
		}
		lenBuf[pos++] = digit;
		llen++;
	} while(len>0);

	uint8_t header = MQTTSUBSCRIBE|MQTTQOS1;
	buffer[4-llen] = header;
	for (int i=0;i<llen;i++) {
		buffer[5-llen+i] = lenBuf[i];
	}

	//packing
	MqttPack pack;
	pack.len = length-5+1+llen;
	memcpy(pack.p, buffer+(4-llen), pack.len);

	for(int i=0;i<pack.len;i++){
		UC15Uart_send_byte(pack.p[i], USART1);
		UC15Uart_send_byte('\n', USART1);
	}

	return pack;
}