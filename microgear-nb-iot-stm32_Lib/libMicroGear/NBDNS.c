#include "NBDNS.h"
#include <string.h>
#include <stdlib.h>

const char *DNS_DEFAULT_SERVER = "8.8.8.8";
#define DNS_PORT 53

#define SOCKET_NONE	255
#define UDP_HEADER_SIZE	8
#define DNS_HEADER_SIZE	12
#define TTL_SIZE        4
#define QUERY_FLAG               (0)
#define RESPONSE_FLAG            (1<<15)
#define QUERY_RESPONSE_MASK      (1<<15)
#define OPCODE_STANDARD_QUERY    (0)
#define OPCODE_INVERSE_QUERY     (1<<11)
#define OPCODE_STATUS_REQUEST    (2<<11)
#define OPCODE_MASK              (15<<11)
#define AUTHORITATIVE_FLAG       (1<<10)
#define TRUNCATION_FLAG          (1<<9)
#define RECURSION_DESIRED_FLAG   (1<<8)
#define RECURSION_AVAILABLE_FLAG (1<<7)
#define RESP_NO_ERROR            (0)
#define RESP_FORMAT_ERROR        (1)
#define RESP_SERVER_FAILURE      (2)
#define RESP_NAME_ERROR          (3)
#define RESP_NOT_IMPLEMENTED     (4)
#define RESP_REFUSED             (5)
#define RESP_MASK                (15)
#define TYPE_A                   (0x0001)
#define CLASS_IN                 (0x0001)
#define LABEL_COMPRESSION_MASK   (0xC0)
// Port number that DNS servers listen on
#define DNS_PORT        53

// Possible return codes from ProcessResponse
#define SUCCESS           1
#define TIMED_OUT        -1
#define INVALID_SERVER   -2
#define TRUNCATED        -3
#define INVALID_RESPONSE -4

#define htons(x) ( ((x)<< 8 & 0xFF00) | \
                   ((x)>> 8 & 0x00FF) )
#define ntohs(x) htons(x)

#define htonl(x) ( ((x)<<24 & 0xFF000000UL) | \
                   ((x)<< 8 & 0x00FF0000UL) | \
                   ((x)>> 8 & 0x0000FF00UL) | \
                   ((x)>>24 & 0x000000FFUL) )
#define ntohl(x) htonl(x)

static inline void Delay_1us(uint32_t nCnt_1us)
{
  volatile uint32_t nCnt;

  for (; nCnt_1us != 0; nCnt_1us--)
    for (nCnt = 13; nCnt != 0; nCnt--);
}

bool DNSInit(DNSClient* dns, UDPConnection* udp){
	if(dns == NULL){
		return false;
	}
	if(udp == NULL){
		return false;
	}

	memset(dns->Cache, 0, DNS_CACHE_SLOT*sizeof(DNSCache));
	dns->CurrentCache = 0;
	memset(dns->ServerIP, 0, 20);
	sprintf(dns->ServerIP, "%s", DNS_DEFAULT_SERVER);
	dns->RequestID = 0;
	dns->UDP = udp;
	return true;
}

bool DNSNumeric(DNSClient* dns, const char* Hostname, char* Result){
	const char* p;
	p = Hostname;

	char cmd[40];
	do{
		if( *p == '.' ){
			sprintf(cmd, "*p: %c\r\n", *p);
			//NBUartSendDEBUG(dns->UDP->NB, cmd);
			p++;
		} else if( *p >= '0' && *p <= '9'){
			sprintf(cmd, "*p: %c\r\n", *p);
			//NBUartSendDEBUG(dns->UDP->NB, cmd);
			p++;
		}else{
			break;
		}
	}while(*p != '\0');

	if (*p == '\0'){
		// we haven't found any invalid characters
		//NBUartSendDEBUG(dns->UDP->NB, "WTF");
		sprintf(Result,"%s", Hostname);
		return true;
	}
	else{
		return false;
	}
}

void DNSInsertCache(DNSClient* dns, char* domain, char* ip){
	if(strlen(domain) < DNS_CACHE_SIZE){
		strcpy(dns->Cache[dns->CurrentCache].Domain, domain);
		strcpy(dns->Cache[dns->CurrentCache].IP, ip);

		dns->CurrentCache = (dns->CurrentCache +1) % DNS_CACHE_SLOT;
	}
}

void DNSClearCache(DNSClient* dns){
	for(uint8_t i=0; i< DNS_CACHE_SLOT; i++){
		dns->Cache[i].Domain[0] = '\0';
	}
	dns->CurrentCache = 0;
}

bool DNSBuildPacket(DNSClient* dns, const char* Hostname){
	// Build header
    //                                    1  1  1  1  1  1
    //      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    //    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //    |                      ID                       |
    //    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //    |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    //    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //    |                    QDCOUNT                    |
    //    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //    |                    ANCOUNT                    |
    //    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //    |                    NSCOUNT                    |
    //    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //    |                    ARCOUNT                    |
    //    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // As we only support one request at a time at present, we can simplify
    // some of this header
	//NBUartSendDEBUG(dns->UDP->NB, "\r\n Start building packet: header\r\n");
    // generate a random ID
    dns->RequestID = rand();
    uint16_t twoByteBuffer;

    // ID
    UDPWrite(dns->UDP, (uint8_t*)&(dns->RequestID), sizeof(dns->RequestID));
	
    // QR Opcode AA TC RD RA Z RCODE
    twoByteBuffer = htons(QUERY_FLAG | OPCODE_STANDARD_QUERY | RECURSION_DESIRED_FLAG);
	UDPWrite(dns->UDP, (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));

	// QDCOUNT
	twoByteBuffer = htons(1);
	UDPWrite(dns->UDP, (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));

	// Zero answer records
	twoByteBuffer = 0;
	UDPWrite(dns->UDP, (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));

	UDPWrite(dns->UDP, (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));

	// and zero additional recored
	UDPWrite(dns->UDP, (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));

	//NBUartSendDEBUG(dns->UDP->NB, "\r\n Start building packet: question\r\n");
	// Build question
	char host[200];
	sprintf(host, "%s", Hostname);
	uint8_t len=0;
	// Run through the name being requested
	char* token = strtok(host, ".");
	while( token != NULL){
		// Write out the size of this section
		len = strlen(token);
		UDPWrite(dns->UDP, &len, sizeof(len));

		// And then write out the section
		UDPWrite(dns->UDP, (uint8_t*)token, (len)*sizeof(uint8_t));

		//NBUartSendDEBUG(dns->UDP->NB, token);
		//NBUartSendDEBUG(dns->UDP->NB, "\r\n");
		token = strtok(NULL, ".");
	}

	//NBUartSendDEBUG(dns->UDP->NB, "\r\n Start building packet: end\r\n");
	// We've got to the end of the question name, so
    // terminate it with a zero-length section
    len = 0;

    UDPWrite(dns->UDP, &len, sizeof(len));

    twoByteBuffer = htons(TYPE_A);
    UDPWrite(dns->UDP, (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));

    twoByteBuffer = htons(CLASS_IN);
    UDPWrite(dns->UDP, (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));

    //NBUartSendDEBUG(dns->UDP->NB, "\r\n DNS building packet: done\r\n");
	return true;
}

bool DNSParseResponse(DNSClient* dns, char* Result){

	int len = UDPParsePacket(dns->UDP);
	char cmd[50];
	sprintf(cmd, "len: %d\r\n", len);
	//NBUartSendDEBUG(dns->UDP->NB, cmd);

	if (len > 30){
		uint8_t header[DNS_HEADER_SIZE]; // Enough space to reuse for the DNS header
		
		// Check server
		if(strcmp(dns->ServerIP, UDPGetIP(dns->UDP)) != 0){
			//NBUartSendDEBUG(dns->UDP->NB, "INVALID_SERVER");
			return false;
		}

		if(UDPAvailable(dns->UDP) < DNS_HEADER_SIZE) {
			//NBUartSendDEBUG(dns->UDP->NB, "TRUNCATED");
			return false;
		}

		UDPRead(dns->UDP, header, DNS_HEADER_SIZE);

		uint16_t staging;
		memcpy(&staging, &header[2], sizeof(uint16_t));
		uint16_t header_flags = htons(staging);
		memcpy(&staging, &header[0], sizeof(uint16_t));

		// Check that it's a response to this request
		if ( ( dns->RequestID != staging ) ||
			((header_flags & QUERY_RESPONSE_MASK) != (uint16_t)RESPONSE_FLAG) ) {

			UDPFlush(dns->UDP);

			//NBUartSendDEBUG(dns->UDP->NB, "INVALID_RESPONSE");
			return false;
		}

		// Check for any errors in the response (or in our request)
    	// although we don't do anything to get round these
    	if ( (header_flags & TRUNCATION_FLAG) || (header_flags & RESP_MASK) ) {
        	// Mark the entire packet as read
        	UDPFlush(dns->UDP);

        	//NBUartSendDEBUG(dns->UDP->NB, "INVALID_RESPONSE");
        	return false; //INVALID_RESPONSE;
    	}

    	// And make sure we've got (at least) one answer
    	memcpy(&staging, &header[6], sizeof(uint16_t));
    	uint16_t answerCount = htons(staging);
    	if (answerCount == 0 ) {
        	// Mark the entire packet as read
    		UDPFlush(dns->UDP);
    		//NBUartSendDEBUG(dns->UDP->NB, "INVALID_RESPONSE");
        	return false; //INVALID_RESPONSE;
    	}

    	// Skip over any questions
    	memcpy(&staging, &header[4], sizeof(uint16_t));
    	for (uint16_t i =0; i < htons(staging); i++) {
        	// Skip over the name
        	uint8_t len;
        	do {
           		UDPRead(dns->UDP, &len, sizeof(len));
            	if (len > 0) {
                	// Don't need to actually read the data out for the string, just
                	// advance ptr to beyond it
                	uint8_t temp;
                	while(len--) {                		
                		UDPRead(dns->UDP, &temp, sizeof(temp));
                    }
            	}
        	} while (len != 0);

        	// Now jump over the type and class
        	uint8_t temp;
        	for (int i =0; i < 4; i++) {
	            //iUdp.read(); // we don't care about the returned byte
	            UDPRead(dns->UDP, &temp, sizeof(temp));
        	}
    	}

    	// Now we're up to the bit we're interested in, the answer
    	// There might be more than one answer (although we'll just use the first
    	// type A answer) and some authority and additional resource records but
    	// we're going to ignore all of them.
		for (uint16_t i =0; i < answerCount; i++) {
	    	// Skip the name
			uint8_t len;
			do {
				UDPRead(dns->UDP, &len, sizeof(len));
				if ((len & LABEL_COMPRESSION_MASK) == 0) {
	            	// It's just a normal label
					if (len > 0) {
	                	// And it's got a length
	                	// Don't need to actually read the data out for the string,
	                	// just advance ptr to beyond it
	                	uint8_t temp;
						while(len--) {
	                    	//iUdp.read(); // we don't care about the returned byte
							UDPRead(dns->UDP, &temp, sizeof(temp));
						}
					}
				}
				else {
		            // This is a pointer to a somewhere else in the message for the
		            // rest of the name.  We don't care about the name, and RFC1035
		            // says that a name is either a sequence of labels ended with a
		            // 0 length octet or a pointer or a sequence of labels ending in
		            // a pointer.  Either way, when we get here we're at the end of
		            // the name
		            // Skip over the pointer
		            //iUdp.read(); // we don't care about the returned byte
		            uint8_t temp;
					UDPRead(dns->UDP, &temp, sizeof(temp));
	            	// And set len so that we drop out of the name loop
					len = 0;
				}
			} while (len != 0);

	    	// Check the type and class
			uint16_t answerType;
			uint16_t answerClass;
			
			UDPRead(dns->UDP, (uint8_t*)&answerType, sizeof(answerType));

			UDPRead(dns->UDP, (uint8_t*)&answerClass, sizeof(answerClass));

	    	// Ignore the Time-To-Live as we don't do any caching
	    	uint8_t temp;
			for (int i =0; i < TTL_SIZE; i++) {
	        	//iUdp.read(); // we don't care about the returned byte
				UDPRead(dns->UDP, &temp, sizeof(temp));
			}

	    	// And read out the length of this answer
	    	// Don't need header_flags anymore, so we can reuse it here
			
			UDPRead(dns->UDP, (uint8_t*)&header_flags, sizeof(header_flags));

			if ( (htons(answerType) == TYPE_A) && (htons(answerClass) == CLASS_IN) ) {
				if (htons(header_flags) != 4) {
		            // It's a weird size
		            // Mark the entire packet as read		            
					UDPRead(dns->UDP, &temp, sizeof(temp));
	            	return false;//INVALID_RESPONSE;
	        	}

	        	//sprintf(cmd, "read then SUCCESS..\r\n");
	        	//BC95_usart_puts(cmd, USART2);
	        	//iUdp.read(aAddress.raw_address(), 4);
	        	uint8_t aAddress[4];
	        	UDPRead(dns->UDP, (uint8_t*)aAddress, 4);

	        	char cmd[20];
	        	sprintf(cmd,"%d.%d.%d.%d", aAddress[0], aAddress[1], aAddress[2], aAddress[3]);

	        	sprintf(Result, "%s", cmd);
		        /*for(int i=0; i<4;i++){
		            sprintf(cmd, "%02X ", aAddress[i]);
		            BC95_usart_puts(cmd, USART2);
		        }*/

		        //sprintf(cmd, "\r\nSUCCESS..\r\n");
		        //BC95_usart_puts(cmd, USART2);
		        return true;
	    	}
		    else {
		        // This isn't an answer type we're after, move onto the next one
		    	uint8_t temp;
		    	for (uint16_t i =0; i < htons(header_flags); i++) {
		            //iUdp.read(); // we don't care about the returned byte

		            //sprintf(cmd, "read for header_flags..\r\n");
		            //BC95_usart_puts(cmd, USART2);
		    		UDPRead(dns->UDP, &temp, sizeof(temp));
		    	}
		    }
		}
		return false;
	}
	return false;
}

int DNSSolve(DNSClient* dns, const char* Hostname, char* Result){
	int ret = 0;
	uint8_t times = 0;

	// check if it is a numeric IP address
	if(DNSNumeric(dns, Hostname, Result)){
		// NBUartSendDEBUG(dns->UDP->NB, "\r\nnumeric Hostname\r\n");
		return 1;
	}

	// check cache
	for (uint8_t i = 0; i < DNS_CACHE_SLOT; i++)
	{
		if(strcmp(dns->Cache[i].Domain, Hostname) == 0){
			sprintf(Result, "%s", dns->Cache[i].IP);

			// NBUartSendDEBUG(dns->UDP->NB, "\r\nfound cache\r\n");
			return 1;
		}
	}

	//NBUartSendDEBUG(dns->UDP->NB, "\r\nno cache\r\n");
	// build DNS packet
	do{
		// UDP connect
		bool p;
		p = UDPConnect(dns->UDP, dns->ServerIP, 8053);
		if(p){
			//NBUartSendDEBUG(dns->UDP->NB, "\r\nUDP connect ok\r\n");
		} else {
			//NBUartSendDEBUG(dns->UDP->NB, "\r\nUDP connect error\r\n");
			return 0;
		}

		// Set Destination Port
		p = UDPSetIP(dns->UDP, dns->ServerIP);
		p = UDPSetPort(dns->UDP, DNS_PORT);
		if(!p){
			UDPFlush(dns->UDP);
			return 0;
		}

		UDPFlushOutbound(dns->UDP);
		// Build DNS packet
		//NBUartSendDEBUG(dns->UDP->NB, "Building Packet\r\n");
		//NBUartSendDEBUG(dns->UDP->NB, (char*)Hostname);
		p = DNSBuildPacket(dns, Hostname);
		//NBUartSendDEBUG(dns->UDP->NB, "Building Packet done\r\n");
		// Send DNS packet
		UDPSendPacket(dns->UDP);
		//NBUartSendDEBUG(dns->UDP->NB,out);

		Delay_1us(100000);

		// Wait for Response
		int retries = 0;
		while(retries < 3 ){
			//NBUartSendDEBUG(dns->UDP->NB, "Parse Packet\r\n");
			p = DNSParseResponse(dns, Result);
			//NBUartSendDEBUG(dns->UDP->NB, "Parse Packet done\r\n");
			if(p){
				ret = 1;
				//NBUartSendDEBUG(dns->UDP->NB, "Packet complete\r\n");
				//NBUartSendDEBUG(dns->UDP->NB, Result);
				//NBUartSendDEBUG(dns->UDP->NB, "\r\n");
				break;
			}
			Delay_1us(10000);
			retries++;
		}

		// UDP disconnect
		p = UDPDisconnect(dns->UDP);
		if(p){
			//NBUartSendDEBUG(dns->UDP->NB, "\r\nUDP disconnect ok\r\n");
		} else {
			//NBUartSendDEBUG(dns->UDP->NB, "\r\nUDP disconnect error\r\n");
			return 0;
		}
		// counter
		times++;
	}while(ret != 1 && times < DNS_MAX_RETRY );

	// insert DNS cache
	if(ret){
		DNSInsertCache(dns, (char*)Hostname, Result);
	}

	return ret;
}

bool DNSIsReboot(DNSClient* dns){
	return UDPIsReboot(dns->UDP);
}