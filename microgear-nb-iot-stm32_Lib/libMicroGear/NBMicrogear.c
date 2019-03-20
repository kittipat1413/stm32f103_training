#include "NBMicrogear.h"

bool MicrogearInit(Microgear* mg, UDPConnection* udp, UDPConnection* dns_udp, char *host, char *appid, char *key, char *secret){
	if(mg == NULL){
		return false;
	}

	if(udp == NULL){
		return false;
	}

	if(dns_udp == NULL){
		return false;
	}

	//point to
	mg->UDP = udp;
	mg->DNS_UDP = dns_udp;

	//assign
	mg->appid = appid;
	mg->key = key;
	mg->secret = secret;

	strcpy(mg->gwaddr, host);

	bool ret = CoAPInit(&mg->CoAP, mg->UDP, mg->DNS_UDP);
	if(!ret){
		return false;
	}

	ret = CoAPStart(&mg->CoAP);
	if(!ret){
		return false;
	}
	return true;
}

uint16_t MicrogearSend(Microgear *mg, char *buffer, char* payload){

	// NBUartSendDEBUG(mg->UDP->NB,"\r\nMicrogear: gwaddr: ");
	// NBUartSendDEBUG(mg->UDP->NB, mg->gwaddr);
	// NBUartSendDEBUG(mg->UDP->NB,"\r\n");
	
	//CoAP_put(Coap *coap, char *ip, int port, char *url, char *payload);
	return CoAPPut(&mg->CoAP, mg->gwaddr, GWPORT, buffer, payload);
}

uint16_t MicrogearPublishString(Microgear *mg, char *topic, char *payload){
	//strcpy_P(mg->buffer, PSTR("topic/"));
    strcpy(mg->buffer, "topic/");
    strcat(mg->buffer, mg->appid);
    strcat(mg->buffer, topic);
    //strcat_P(mg->buffer, PSTR("?auth="));
    strcat(mg->buffer, "?auth=");
    strcat(mg->buffer, mg->key);
    strcat(mg->buffer, ":");
    strcat(mg->buffer, mg->secret);
    return MicrogearSend(mg, mg->buffer, payload);
}

uint16_t MicrogearPublishInt(Microgear *mg, char *topic, int value){
	//itoa(value, mg->buffer+buffersize-8, 10);
	sprintf(mg->buffer+buffersize-8,"%d", value);
    return MicrogearPublishString(mg, topic, mg->buffer+buffersize-8);
}

void MicrogearChat(Microgear *mg, char *alias, char *payload){
	//strcpy_P(mg->buffer, PSTR("microgear/"));
	strcpy(mg->buffer, "microgear/");
    strcat(mg->buffer, mg->appid);
    strcat(mg->buffer, "/");
    strcat(mg->buffer, alias);
    //strcat_P(mg->buffer, PSTR("?auth="));
    strcat(mg->buffer, "?auth=");
    strcat(mg->buffer, mg->key);
    strcat(mg->buffer, ":");
    strcat(mg->buffer, mg->secret);
    MicrogearSend(mg, mg->buffer, payload);
}

void MicrogearWriteFeed(Microgear *mg, char *feedid, char *payload){
	char *p;

    //strcpy_P(buffer, PSTR("feed/"));
    strcpy(mg->buffer, "feed/");
    strcat(mg->buffer, feedid);
    //strcat_P(buffer, PSTR("?auth="));
    strcat(mg->buffer, "?auth=");
    strcat(mg->buffer, mg->key);
    strcat(mg->buffer, ":");
    strcat(mg->buffer, mg->secret);

    p = mg->buffer + strlen(mg->buffer) + 1;
    //strcpy_P(p, PSTR("data="));
    strcpy(p, "data=");
    strcat(p, payload);
    MicrogearSend(mg, mg->buffer, p);
}

void MicrogearWriteFeedAPIkey(Microgear *mg, char *feedid, char *payload, char *apikey){
	char *p;

    //strcpy_P(buffer, PSTR("feed/"));
    strcpy(mg->buffer, "feed/");
    strcat(mg->buffer, feedid);
    //strcat_P(buffer, PSTR("?apikey="));
    strcat(mg->buffer, "?apikey=");
    strcat(mg->buffer, apikey);

    p = mg->buffer + strlen(mg->buffer) + 1;
    //strcpy_P(p, PSTR("data="));
    strcpy(p , "data=");
    strcat(p, payload);

    MicrogearSend(mg, mg->buffer, p);
}

void MicrogearPushOwner(Microgear *mg, char *text){
	strcpy(mg->buffer, "push/owner?auth=");
    strcat(mg->buffer, mg->key);
    strcat(mg->buffer, ":");
    strcat(mg->buffer, mg->secret);
    MicrogearSend(mg, mg->buffer, text);
}

void Microgear_loop(Microgear *mg){
	CoAPLoop(&mg->CoAP);
}

bool MicrogearIsReboot(Microgear *mg){
    return CoAPIsReboot(&mg->CoAP);
}