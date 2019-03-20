#ifndef NBDNS_H_
#define NBDNS_H_

#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "NBQueue.h"
#include "NBUart.h"

#define DNS_CACHE_SLOT 2
#define DNS_CACHE_SIZE 100
#define DNS_MAX_RETRY 5

extern const char *DNS_DEFAULT_SERVER;

typedef struct DNSCache {
	char Domain[DNS_CACHE_SIZE];
	char IP[20];
} DNSCache;

typedef struct DNSClient {
	DNSCache Cache[DNS_CACHE_SLOT];
	uint8_t CurrentCache;
	char ServerIP[20];
	uint16_t RequestID;
	UDPConnection* UDP;
} DNSClient;

bool DNSInit(DNSClient* dns, UDPConnection* udp);
int DNSSolve(DNSClient* dns, const char* Hostname, char* Result);

bool DNSIsReboot(DNSClient* dns);
#endif