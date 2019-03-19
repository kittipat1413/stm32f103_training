#include "BC95.h"
#include "BC95Udp.h"
#include "Dns.h"
#include "MicrogearNB.h"

void Microgear_udp_init(Microgear *mg, BC95UDP *udp) {
	mg->udp = udp;
}
void Microgear_init(Microgear *mg, char *appid, char *key, char *secret){
	mg->appid = appid;
	mg->key = key;
	mg->secret = secret;
}
void Microgear_begin(Microgear *mg, uint16_t port){
	DNS_CLIENT dns;
	DNS_CLIENT_init(&dns, mg->udp->bc95);
	DNS_CLIENT_begin(&dns, "8.8.8.8");

	char remote_addr[8];
	char *premote_addr;
	premote_addr = remote_addr;

	DNS_CLIENT_getHostByName(&dns, GWHOST, premote_addr);

	sprintf(remote_addr,"%d.%d.%d.%d", remote_addr[0], remote_addr[1], remote_addr[2], remote_addr[3]);
	mg->gwaddr = premote_addr;

	BC95Udp_begin(mg->udp, port);

	Coap coap;
	CoAP_init(&coap, mg->udp);
	mg->coap = &coap;
}
void Microgear_publish_int(Microgear *mg, char *topic, int value){
	//itoa(value, mg->buffer+buffersize-8, 10);
	sprintf(mg->buffer+buffersize-8,"%d", value);
    Microgear_publish_string(mg, topic, mg->buffer+buffersize-8);
}
void Microgear_publish_string(Microgear *mg, char *topic, char *payload){
	//strcpy_P(mg->buffer, PSTR("topic/"));
    strcpy(mg->buffer, "topic/");
    strcat(mg->buffer, mg->appid);
    strcat(mg->buffer, topic);
    //strcat_P(mg->buffer, PSTR("?auth="));
    strcat(mg->buffer, "?auth=");
    strcat(mg->buffer, mg->key);
    strcat(mg->buffer, ":");
    strcat(mg->buffer, mg->secret);
    Microgear_coapSend(mg, mg->buffer, payload);
}
void Microgear_chat(Microgear *mg, char *alias, char *payload){
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
    Microgear_coapSend(mg, mg->buffer, payload);
}
void Microgear_writeFeed(Microgear *mg, char *feedid, char *payload){
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
    Microgear_coapSend(mg, mg->buffer, p);
}
void Microgear_writeFeed_APIkey(Microgear *mg, char *feedid, char *payload, char *apikey){
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

    Microgear_coapSend(mg, mg->buffer, p);
}
void Microgear_pushOwner(Microgear *mg, char *text){
	strcpy(mg->buffer, "push/owner?auth=");
    strcat(mg->buffer, mg->key);
    strcat(mg->buffer, ":");
    strcat(mg->buffer, mg->secret);
    Microgear_coapSend(mg, mg->buffer, text);
}

void Microgear_loop(Microgear *mg){
	CoAP_loop(mg->coap);
}

void Microgear_coapSend(Microgear *mg, char *buffer, char* payload){
	//CoAP_put(Coap *coap, char *ip, int port, char *url, char *payload);
	CoAP_put(mg->coap, mg->gwaddr, GWPORT, buffer, payload);
}