#include "UC15TCP.h"

static inline void Delay_1us(uint32_t nCnt_1us)
{
  volatile uint32_t nCnt;

  for (; nCnt_1us != 0; nCnt_1us--)
    for (nCnt = 13; nCnt != 0; nCnt--);
}

bool UC15TCPInit(TCP* tcp, UC15Uart* uc){
	tcp->uc = uc;
	return true;
}

bool UC15TCPOpen(TCP* tcp, unsigned char contexid, unsigned char connectid, char* service_type, char* ip_url, char* remote_port, char* local_port, unsigned char access_mode){
	// const long interval = 1000;
	// AT+QIOPEN=1,0,"TCP","www.settakan.com",80,0,0
	char out[100];
	sprintf(out, "AT+QIOPEN=%c,%c,\"%s\",\"%s\",%s,%s,%c\r\n", contexid, connectid, service_type, ip_url, remote_port, local_port, access_mode);
	UC15UartSendDEBUG(tcp->uc, out);
	UC15UartSend(tcp->uc, out);

	Delay_1us(1000000);

	tcp->uc->external_topic = true;
	tcp->uc->topic = "+QIOPEN";

	int8_t ret = UC15UartRead(tcp->uc);
	if(ret == VALIDMESSAGE_END_OK){
		// UC15UartGetString(tcp->uc, out);
		return true;
	}

	tcp->uc->external_topic = false;
	tcp->uc->topic = NULL;

	// UC15UartGetString(tcp->uc, out);
	UC15UartSendDEBUG(tcp->uc, "tcp open fail\r\n");
	return false;
}

bool UC15TCPState(TCP* tcp, unsigned char connectid){
	char out[200];
	sprintf(out, "AT+QISTATE=1,%c\r\n", connectid);
	UC15UartSendDEBUG(tcp->uc, out);
	UC15UartSend(tcp->uc, out);

	Delay_1us(100000);

	tcp->uc->external_topic = true;
	tcp->uc->topic = "+QISTATE";

	int8_t ret = UC15UartRead(tcp->uc);
	if(ret == VALIDMESSAGE_END_OK){
		tcp->uc->external_topic = false;
		tcp->uc->topic = NULL;

		int8_t ret = UC15UartRead(tcp->uc);
		if(ret == VALIDMESSAGE_END_OK){
			return true;
		}
		// UC15UartGetString(tcp->uc, out);
		UC15UartSendDEBUG(tcp->uc, "tcp state fail, no end\r\n");
		return false;
	}

	tcp->uc->external_topic = false;
	tcp->uc->topic = NULL;

	// UC15UartGetString(tcp->uc, out);
	UC15UartSendDEBUG(tcp->uc, "tcp state fail\r\n");
	return false;
}

bool UC15TCPStartSend(TCP* tcp, unsigned char connectid){
	char out[200];
	sprintf(out, "AT+QISEND=%c\r\n", connectid);
	UC15UartSendDEBUG(tcp->uc, out);
	UC15UartSend(tcp->uc, out);

	Delay_1us(100000);

	int8_t ret = UC15UartRead(tcp->uc);
	if(ret == READY_TO_SEND){
		UC15UartSendDEBUG(tcp->uc, "\nready to send!\n");
		// UC15UartGetString(tcp->uc, out);
		return true;
	}
	return false;
}

bool UC15TCPSend(TCP* tcp, char* data){
	UC15UartSend(tcp->uc, data);
	return true;
}

bool UC15TCPStopSend(TCP* tcp){
	char out[50];
	out[0] = 0x1A;
	out[1] = '\r';
	out[2] = '\n';
	out[3] = '\0';
	UC15UartSend(tcp->uc, out);
	Delay_1us(100000);

	tcp->uc->external_topic = true;
	tcp->uc->topic = "SEND OK";

	int8_t ret = UC15UartRead(tcp->uc);
	if(ret == VALIDMESSAGE_END_OK){
		// UC15UartGetString(tcp->uc, out);
		return true;
	}

	tcp->uc->external_topic = false;
	tcp->uc->topic = NULL;

	// UC15UartGetString(tcp->uc, out);
	UC15UartSendDEBUG(tcp->uc, "tcp stop send fail\r\n");
	return false;
}

int UC15TCPReadBuffer(TCP* tcp, unsigned char connectid){
	char out[200];
	sprintf(out, "AT+QIRD=%c,1500\r\n", connectid);
	UC15UartSendDEBUG(tcp->uc, out);
	UC15UartSend(tcp->uc, out);

	Delay_1us(100000);

	tcp->uc->external_topic = true;
	tcp->uc->topic = "+QIRD";

	int8_t ret = UC15UartRead(tcp->uc);
	if(ret == VALIDMESSAGE_END_OK){
		UC15UartGetString(tcp->uc, out);
		UC15UartSendDEBUG(tcp->uc, out);

		tcp->uc->topic = "OK";
		int8_t ret = UC15UartRead(tcp->uc);
		if(ret == VALIDMESSAGE_END_OK){
			UC15UartGetString(tcp->uc, out);
			int len = strlen(out);
			out[len-8] = '\0';

			UC15UartSendDEBUG(tcp->uc, out);
		}
		return true;
	}

	tcp->uc->external_topic = false;
	tcp->uc->topic = NULL;

	UC15UartGetString(tcp->uc, out);
	UC15UartSendDEBUG(tcp->uc, out);
	UC15UartSendDEBUG(tcp->uc, "tcp stop send fail\r\n");
	return false;
}

bool UC15TCPReceiveAvailable(TCP* tcp){
	tcp->uc->external_topic = true;
	tcp->uc->topic = "+QIURC";

	char out[100];

	int8_t ret = UC15UartRead(tcp->uc);
	if(ret == VALIDMESSAGE_END_OK){
		UC15UartGetString(tcp->uc, out);
		return true;
	}

	tcp->uc->external_topic = false;
	tcp->uc->topic = NULL;

	UC15UartGetString(tcp->uc, out);
	UC15UartSendDEBUG(tcp->uc, out);
	UC15UartSendDEBUG(tcp->uc, "tcp receive fail\r\n");
	UC15UartSendDEBUG(tcp->uc, tcp->uc->buffer);
	UC15UartSendDEBUG(tcp->uc, "\r\n\r\n");
	return false;
}

bool UC15TCPCloseWithContex(TCP* tcp, unsigned char contexid){
	char out[50];
	sprintf(out, "AT+QICLOSE=%c\r\n", contexid);
	UC15UartSend(tcp->uc, out);
	Delay_1us(1000000);
	
	tcp->uc->external_topic = true;
	tcp->uc->topic = "OK";

	int8_t ret = UC15UartRead(tcp->uc);
	if(ret == VALIDMESSAGE_END_OK){
		// UC15UartGetString(tcp->uc, out);
		return true;
	}

	tcp->uc->external_topic = false;
	tcp->uc->topic = NULL;

	UC15UartGetString(tcp->uc, out);
	UC15UartSendDEBUG(tcp->uc, "tcp close fail\r\n");
	return false;
}

bool UC15TCPClose(TCP* tcp){
	return UC15TCPCloseWithContex(tcp, '0');
}