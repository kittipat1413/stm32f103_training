#ifndef Dns_H_
#define Dns_H_

#include "BC95Udp.h"

#define DNS_CACHE_SLOT                  1
#define DNS_CACHE_SIZE                  24
#define DNS_CACHE_EXPIRE_MS             0
#define DNS_MAX_RETRY                   5

extern const char *DNS_DEFAULT_SERVER;// = "8.8.8.8";

#if DNS_CACHE_SLOT > 0
struct DNS_CACHE_STRUCT {
    char domain[DNS_CACHE_SIZE];
    char *ip;
};
typedef struct DNS_CACHE_STRUCT dns_cache_struct;
#endif

typedef struct DNS_CLIENT
{
	dns_cache_struct dnscache[DNS_CACHE_SLOT];
	char *iDNSServer;
	uint16_t iRequestID;
	BC95UDP iUDP;
} DNS_CLIENT;

void DNS_CLIENT_init(DNS_CLIENT* dns_client, BC95_str* bc95);
void DNS_CLIENT_begin(DNS_CLIENT* dns_client, char* aDNSServer);
int DNS_CLIENT_inet_aton(DNS_CLIENT* dns_client, const char* aIPAddrString, char* aResult);
int DNS_CLIENT_getHostByName(DNS_CLIENT* dns_client, const char* aHostname, char* aResult);

uint16_t DNS_CLIENT_BuildRequest(DNS_CLIENT* dns_client, const char* aName);
uint16_t DNS_CLIENT_ProcessResponse(DNS_CLIENT* dns_client, uint16_t aTimeout, char* aAddress);

void DNS_CLIENT_insertDNSCache(DNS_CLIENT* dns_client, char* domain, char* ip);
void DNS_CLIENT_clearDNSCache(DNS_CLIENT* dns_client);

#endif