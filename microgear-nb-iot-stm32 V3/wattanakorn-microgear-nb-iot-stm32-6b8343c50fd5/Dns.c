#include "Dns.h"
#include <string.h>

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

static uint8_t current_dns_slot = 0;
const char *DNS_DEFAULT_SERVER = "8.8.8.8";

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

void DNS_CLIENT_init(DNS_CLIENT* dns_client, BC95_str* bc95){
    BC95Udp_init(&(dns_client->iUDP), bc95);
}

void DNS_CLIENT_begin(DNS_CLIENT* dns_client, char* aDNSServer){
	dns_client->iDNSServer = aDNSServer;
	dns_client->iRequestID = 0;
}

int DNS_CLIENT_inet_aton(DNS_CLIENT* dns_client, const char* aIPAddrString, char* aResult){
	const char* p = aIPAddrString;
	while (*p && ( (*p == '.') || (*p >= '0') || (*p <= '9') )) {
        p++;
    }

    if (*p == '\0') {
        // It's looking promising, we haven't found any invalid characters
        p = aIPAddrString;
        int segment =0;
        int segmentValue =0;
        while (*p && (segment < 4)) {
            if (*p == '.') {
                // We've reached the end of a segment
                if (segmentValue > 255) {
                    // You can't have IP address segments that don't fit in a byte
                    return 0;
                }
                else {
                    aResult[segment] = (char)segmentValue;
                    segment++;
                    segmentValue = 0;
                }
            }
            else {
                // Next digit
                segmentValue = (segmentValue*10)+(*p - '0');
            }
            p++;
        }
        // We've reached the end of address, but there'll still be the last
        // segment to deal with
        if ((segmentValue > 255) || (segment > 3)) {
            // You can't have IP address segments that don't fit in a byte,
            // or more than four segments
            return 0;
        }
        else {
            aResult[segment] = (char)segmentValue;
            return 1;
        }
    }
    else {
        return 0;
    }
}


int DNS_CLIENT_getHostByName(DNS_CLIENT* dns_client, const char* aHostname, char* aResult){
	int ret = 0;
    uint8_t times = 0;

    char cmd[300];
    // See if it's a numeric IP address
    if (DNS_CLIENT_inet_aton(dns_client, aHostname, aResult)) {
        // It is, our work here is done
        sprintf(cmd, "dns skip\r\n");
        BC95_usart_puts(cmd, USART2);
        return 1;
    }

    #if DNS_CACHE_SLOT > 0

        #if DNS_CACHE_EXPIRE_MS > 0
            if (millis() > dns_expire_timestamp) {
                clearDNSCache();
            }
        #endif

        for (uint8_t i=0 ; i<DNS_CACHE_SLOT; i++) {
            if (strcmp(dns_client->dnscache[current_dns_slot].domain, aHostname)==0) {
                aResult = dns_client->dnscache[current_dns_slot].ip;
                return 1;
            }
        }
    #endif
    
    do {
        BC95Udp_begin(&(dns_client->iUDP),8053);

        int retries = 0;

        sprintf(cmd, "dns: host is %s\r\n",aHostname);
        BC95_usart_puts(cmd, USART2);

        sprintf(cmd, "%s %d\r\n",dns_client->iDNSServer, DNS_PORT);
        BC95_usart_puts(cmd, USART2);
        
        // Send DNS request
        ret = BC95Udp_beginPacket(&(dns_client->iUDP), dns_client->iDNSServer, DNS_PORT);
        sprintf(cmd, "beginPacket..\r\n");
        BC95_usart_puts(cmd, USART2);

        if (ret != 0) {
            // Now output the request data
            ret =  DNS_CLIENT_BuildRequest(dns_client, aHostname);
            sprintf(cmd, "BuildRequest..\r\n");
            BC95_usart_puts(cmd, USART2);
            if (ret != 0) {
                // And finally send the request
                sprintf(cmd, "try endPacket..\r\n");
                BC95_usart_puts(cmd, USART2);
                ret = (int)BC95Udp_endPacket(&(dns_client->iUDP));
                sprintf(cmd, "endPacket..\r\n");
                BC95_usart_puts(cmd, USART2);

                if (ret != 0) {
                    // Now wait for a response
                    int wait_retries = 0;
                    ret = TIMED_OUT;
                    while ((wait_retries < 3) && (ret == TIMED_OUT)) {
                        sprintf(cmd, "run ProcessResponse %d..\r\n", wait_retries);
                        BC95_usart_puts(cmd, USART2);

                        ret = DNS_CLIENT_ProcessResponse(dns_client, 5000, aResult);
                        wait_retries++;
                    }
                }
            }
        }
        retries++;
        times++;
        BC95Udp_stop(&(dns_client->iUDP));

        sprintf(cmd, "close socket..\r\n");
        BC95_usart_puts(cmd, USART2);

    } while (ret!=1 && times<DNS_MAX_RETRY);
    
    sprintf(cmd, "dns time out\r\n");
    BC95_usart_puts(cmd, USART2);
    
    #if DNS_CACHE_SLOT > 0
        if (ret) {
            //insertDNSCache(aHostname, aResult);
            DNS_CLIENT_insertDNSCache(dns_client, (char*)aHostname, aResult);
            #if DNS_CACHE_EXPIRE_MS > 0
                dns_expire_timestamp = millis() + DNS_CACHE_EXPIRE_MS;
            #endif
        }
    #endif
	
    return ret;
}

uint16_t DNS_CLIENT_BuildRequest(DNS_CLIENT* dns_client, const char* aName) {
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
    //char cmd[100];
    //sprintf(cmd, "run BuildRequest..\r\n");
    //BC95_usart_puts(cmd, USART2);

    dns_client->iRequestID = rand(); // generate a random ID
    uint16_t twoByteBuffer;

    // FIXME We should also check that there's enough space available to write to, rather
    // FIXME than assume there's enough space (as the code does at present)
    //iUdp.write((uint8_t*)&iRequestId, sizeof(iRequestId));
    BC95Udp_write(&(dns_client->iUDP), (uint8_t*)&(dns_client->iRequestID), sizeof(dns_client->iRequestID));

    twoByteBuffer = htons(QUERY_FLAG | OPCODE_STANDARD_QUERY | RECURSION_DESIRED_FLAG);
    //iUdp.write((uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    BC95Udp_write(&(dns_client->iUDP), (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));

    twoByteBuffer = htons(1);  // One question record
    //iUdp.write((uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    BC95Udp_write(&(dns_client->iUDP), (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));

    twoByteBuffer = 0;  // Zero answer records
    //iUdp.write((uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    BC95Udp_write(&(dns_client->iUDP), (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));

    //iUdp.write((uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    BC95Udp_write(&(dns_client->iUDP), (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    
    // and zero additional records
    //iUdp.write((uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    BC95Udp_write(&(dns_client->iUDP), (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));

    //sprintf(cmd, "pbufferlen: %d\r\n", dns_client->iUDP.pbufferlen);
    //BC95_usart_puts(cmd, USART2);

    // Build question
    const char* start =aName;
    const char* end =start;
    uint8_t len;
    // Run through the name being requested
    while (*end) {
        // Find out how long this section of the name is
        end = start;
        while (*end && (*end != '.') ) {
            end++;
        }

        if (end-start > 0) {
            // Write out the size of this section
            len = end-start;
            //iUdp.write(&len, sizeof(len));
            BC95Udp_write(&(dns_client->iUDP), &len, sizeof(len));

            // And then write out the section
            //iUdp.write((uint8_t*)start, end-start);
            BC95Udp_write(&(dns_client->iUDP), (uint8_t*)start, end-start);
        }
        start = end+1;
    }

    // We've got to the end of the question name, so
    // terminate it with a zero-length section
    len = 0;
    //iUdp.write(&len, sizeof(len));
    BC95Udp_write(&(dns_client->iUDP), &len, sizeof(len));
    // Finally the type and class of question
    twoByteBuffer = htons(TYPE_A);
    //iUdp.write((uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    BC95Udp_write(&(dns_client->iUDP), (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));

    twoByteBuffer = htons(CLASS_IN);  // Internet class of question
    //iUdp.write((uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    BC95Udp_write(&(dns_client->iUDP), (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    // Success!  Everything buffered okay
    //sprintf(cmd, "BuildRequest Success..\r\n");
    //BC95_usart_puts(cmd, USART2);

    return 1;
}

uint16_t DNS_CLIENT_ProcessResponse(DNS_CLIENT* dns_client, uint16_t aTimeout, char* aAddress) {
    //uint32_t startTime = millis();
    // Wait for a response packet
    //char cmd[100];
    //sprintf(cmd, "run ProcessResponse..\r\n");
    //BC95_usart_puts(cmd, USART2);

    uint8_t counter = 0;
    while(BC95Udp_parsePacket(&(dns_client->iUDP)) <= 0) {
        //if((millis() - startTime) > aTimeout)
        //    return TIMED_OUT;
        //sprintf(cmd, "try parsePacket %d..\r\n", counter);
        //BC95_usart_puts(cmd, USART2);

        if(counter >= 100){
        	return TIMED_OUT;
        }
        counter++;
        Delay_1us(50000);
    }
    
    //sprintf(cmd, "parsePacket done >> read header..\r\n");
    //BC95_usart_puts(cmd, USART2);

    // We've had a reply!
    // Read the UDP header
    uint8_t header[DNS_HEADER_SIZE]; // Enough space to reuse for the DNS header
    // Check that it's a response from the right server and the right port
    if ( (dns_client->iDNSServer != BC95UDP_remoteIP(&(dns_client->iUDP))) ||
        (BC95UDP_remotePort(&(dns_client->iUDP)) != DNS_PORT) ) {
        // It's not from who we expected
        return INVALID_SERVER;
    }

    // Read through the rest of the response
    if (BC95UDP_available(&(dns_client->iUDP)) < DNS_HEADER_SIZE) {
        return TRUNCATED;
    }
    BC95UDP_read(&(dns_client->iUDP), header, DNS_HEADER_SIZE);

    
    uint16_t staging; // Staging used to avoid type-punning warnings
    memcpy(&staging, &header[2], sizeof(uint16_t));
    uint16_t header_flags = htons(staging);
    memcpy(&staging, &header[0], sizeof(uint16_t));

    //sprintf(cmd, "Staging..\r\n");
    //BC95_usart_puts(cmd, USART2);
    // Check that it's a response to this request
    if ( ( dns_client->iRequestID != staging ) ||
        ((header_flags & QUERY_RESPONSE_MASK) != (uint16_t)RESPONSE_FLAG) ) {
        // Mark the entire packet as read
        BC95UDP_flush(&(dns_client->iUDP));
        return INVALID_RESPONSE;
    }
    // Check for any errors in the response (or in our request)
    // although we don't do anything to get round these
    if ( (header_flags & TRUNCATION_FLAG) || (header_flags & RESP_MASK) ) {
        // Mark the entire packet as read
        BC95UDP_flush(&(dns_client->iUDP));
        return -5; //INVALID_RESPONSE;
    }
    
    // And make sure we've got (at least) one answer
    memcpy(&staging, &header[6], sizeof(uint16_t));
    uint16_t answerCount = htons(staging);
    if (answerCount == 0 ) {
        // Mark the entire packet as read
        BC95UDP_flush(&(dns_client->iUDP));
        return -6; //INVALID_RESPONSE;
    }

    //sprintf(cmd, "Skip over any questions..\r\n");
    //BC95_usart_puts(cmd, USART2);
    // Skip over any questions
    memcpy(&staging, &header[4], sizeof(uint16_t));
    for (uint16_t i =0; i < htons(staging); i++) {
        // Skip over the name
        uint8_t len;
        do {
            //iUdp.read(&len, sizeof(len));
            BC95UDP_read(&(dns_client->iUDP), &len, sizeof(len));
            if (len > 0) {
                // Don't need to actually read the data out for the string, just
                // advance ptr to beyond it
                while(len--) {
                    BC95UDP_read_null(&(dns_client->iUDP)); // we don't care about the returned byte
                }
            }
        } while (len != 0);

        // Now jump over the type and class
        for (int i =0; i < 4; i++) {
            //iUdp.read(); // we don't care about the returned byte
            BC95UDP_read_null(&(dns_client->iUDP));
        }
    }
    
    // Now we're up to the bit we're interested in, the answer
    // There might be more than one answer (although we'll just use the first
    // type A answer) and some authority and additional resource records but
    // we're going to ignore all of them.
    //sprintf(cmd, "answerCount..\r\n");
    //BC95_usart_puts(cmd, USART2);

    for (uint16_t i =0; i < answerCount; i++) {
        // Skip the name
        uint8_t len;
        do {
            //iUdp.read(&len, sizeof(len));
            BC95UDP_read(&(dns_client->iUDP), &len, sizeof(len));
            if ((len & LABEL_COMPRESSION_MASK) == 0) {
                // It's just a normal label
                if (len > 0) {
                    // And it's got a length
                    // Don't need to actually read the data out for the string,
                    // just advance ptr to beyond it
                    while(len--) {
                        //iUdp.read(); // we don't care about the returned byte
                        BC95UDP_read_null(&(dns_client->iUDP));
                    }
                }
                //sprintf(cmd, "normal label..\r\n");
                //BC95_usart_puts(cmd, USART2);
            }
            else {
                //sprintf(cmd, "skip label..\r\n");
                //BC95_usart_puts(cmd, USART2);
                // This is a pointer to a somewhere else in the message for the
                // rest of the name.  We don't care about the name, and RFC1035
                // says that a name is either a sequence of labels ended with a
                // 0 length octet or a pointer or a sequence of labels ending in
                // a pointer.  Either way, when we get here we're at the end of
                // the name
                // Skip over the pointer
                //iUdp.read(); // we don't care about the returned byte
                BC95UDP_read_null(&(dns_client->iUDP));
                // And set len so that we drop out of the name loop
                len = 0;
            }
        } while (len != 0);

        // Check the type and class
        uint16_t answerType;
        uint16_t answerClass;

        //sprintf(cmd, "Check the type..\r\n");
        //BC95_usart_puts(cmd, USART2);

        //iUdp.read((uint8_t*)&answerType, sizeof(answerType));
        BC95UDP_read(&(dns_client->iUDP), (uint8_t*)&answerType, sizeof(answerType));

        //iUdp.read((uint8_t*)&answerClass, sizeof(answerClass));
        BC95UDP_read(&(dns_client->iUDP), (uint8_t*)&answerClass, sizeof(answerClass));

        // Ignore the Time-To-Live as we don't do any caching
        for (int i =0; i < TTL_SIZE; i++) {
            //iUdp.read(); // we don't care about the returned byte
            BC95UDP_read_null(&(dns_client->iUDP));
        }
        
        //sprintf(cmd, "Ignore the Time-To-Live..\r\n");
        //BC95_usart_puts(cmd, USART2);
        // And read out the length of this answer
        // Don't need header_flags anymore, so we can reuse it here
        //iUdp.read((uint8_t*)&header_flags, sizeof(header_flags));
        BC95UDP_read(&(dns_client->iUDP), (uint8_t*)&header_flags, sizeof(header_flags));
        //sprintf(cmd, "read header_flags..\r\n");
        //BC95_usart_puts(cmd, USART2);

        if ( (htons(answerType) == TYPE_A) && (htons(answerClass) == CLASS_IN) ) {
            if (htons(header_flags) != 4) {
                // It's a weird size
                // Mark the entire packet as read
                //sprintf(cmd, "It's a weird size..\r\n");
                //BC95_usart_puts(cmd, USART2);
                //iUdp.flush();
                BC95UDP_flush(&(dns_client->iUDP));
                return -9;//INVALID_RESPONSE;
            }

            //sprintf(cmd, "read then SUCCESS..\r\n");
            //BC95_usart_puts(cmd, USART2);
            //iUdp.read(aAddress.raw_address(), 4);
            BC95UDP_read(&(dns_client->iUDP), (uint8_t*)aAddress, 4);

            /*for(int i=0; i<4;i++){
                sprintf(cmd, "%02X ", aAddress[i]);
                BC95_usart_puts(cmd, USART2);
            }*/

            //sprintf(cmd, "\r\nSUCCESS..\r\n");
            //BC95_usart_puts(cmd, USART2);
            return SUCCESS;
        }
        else {
            //sprintf(cmd, "not answer, move to next one..\r\n");
            //BC95_usart_puts(cmd, USART2);
            // This isn't an answer type we're after, move onto the next one
            for (uint16_t i =0; i < htons(header_flags); i++) {
                //iUdp.read(); // we don't care about the returned byte

                //sprintf(cmd, "read for header_flags..\r\n");
                //BC95_usart_puts(cmd, USART2);
                BC95UDP_read_null(&(dns_client->iUDP));
            }
        }
    }

    //sprintf(cmd, "flushing..\r\n");
    //BC95_usart_puts(cmd, USART2);
    // Mark the entire packet as read
    //iUdp.flush();
    BC95UDP_flush(&(dns_client->iUDP));
	
    // If we get here then we haven't found an answer
    return -10;//INVALID_RESPONSE;
}

#if DNS_CACHE_SLOT > 0
void DNS_CLIENT_insertDNSCache(DNS_CLIENT* dns_client, char* domain, char* ip){
    if (strlen(domain) < DNS_CACHE_SIZE) {
        strcpy(dns_client->dnscache[current_dns_slot].domain, domain);
        dns_client->dnscache[current_dns_slot].ip = ip;
        current_dns_slot = (current_dns_slot+1) % DNS_CACHE_SLOT;
    }
}
void DNS_CLIENT_clearDNSCache(DNS_CLIENT* dns_client){
    for (uint8_t i; i<DNS_CACHE_SLOT; i++) {
        dns_client->dnscache[i].domain[0] = '\0';
    }
    current_dns_slot = 0;
}
#endif