#ifndef DNSPACKET_H_
#define DNSPACKET_H_

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

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

uint16_t DNSBuildPacket(uint16_t* request_id, uint8_t* buffer, const char* Hostname);
bool DNSDecode(uint16_t requestID, uint8_t* packet, uint16_t packet_length, char* result);

uint16_t DNSBuildPacket(uint16_t* request_id, uint8_t* buffer, const char* Hostname){
    uint16_t length = 0;

    uint16_t requestID = rand();
    uint16_t twoByteBuffer;
    //ID
    memcpy(buffer+length, (uint8_t*)&requestID, sizeof(requestID));
    length += sizeof(requestID);

    // QR Opcode AA TC RD RA Z RCODE
    twoByteBuffer = htons(QUERY_FLAG | OPCODE_STANDARD_QUERY | RECURSION_DESIRED_FLAG);
    memcpy(buffer+length, (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    length += sizeof(twoByteBuffer);

    // QDCOUNT
    twoByteBuffer = htons(1);
    memcpy(buffer+length, (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    length += sizeof(twoByteBuffer);

    // Zero answer records
	twoByteBuffer = 0;
    memcpy(buffer+length, (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    length += sizeof(twoByteBuffer);

    memcpy(buffer+length, (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    length += sizeof(twoByteBuffer);

    // and zero additional recored
    memcpy(buffer+length, (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    length += sizeof(twoByteBuffer);
    
    char host[200];
    sprintf(host, "%s", Hostname);
    uint8_t len=0;
	
    // Run through the name being requested
	char* token = strtok(host, ".");
	while( token != NULL){
		// Write out the size of this section
		len = strlen(token);
        memcpy(buffer+length, (uint8_t*)&len, sizeof(len));
        length += sizeof(len);

		// And then write out the section
        memcpy(buffer+length, (uint8_t*)token, (len)*sizeof(uint8_t));
        length += (len)*sizeof(uint8_t);

		//NBUartSendDEBUG(dns->UDP->NB, token);
		//NBUartSendDEBUG(dns->UDP->NB, "\r\n");
		token = strtok(NULL, ".");
	}

    len = 0;

    memcpy(buffer+length, (uint8_t*)&len, sizeof(len));
    length += sizeof(len);

    twoByteBuffer = htons(TYPE_A);
    memcpy(buffer+length, (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    length += sizeof(twoByteBuffer);

    twoByteBuffer = htons(CLASS_IN);
    memcpy(buffer+length, (uint8_t*)&twoByteBuffer, sizeof(twoByteBuffer));
    length += sizeof(twoByteBuffer);

    *request_id = requestID;
    return length;
}

bool DNSDecode(uint16_t requestID, uint8_t* packet, uint16_t packet_length, char* result){
    // char debug[200];
	uint8_t header[DNS_HEADER_SIZE];
	uint8_t* p;
	p = packet;
    memcpy(header, p, DNS_HEADER_SIZE);
	p += DNS_HEADER_SIZE;

	uint16_t staging;
	memcpy(&staging, &header[2], sizeof(uint16_t));
	uint16_t header_flags = htons(staging);
	memcpy(&staging, &header[0], sizeof(uint16_t));
	// Check requestID
	// usart_puts1("check requestID\n");

    if ( ( requestID != staging ) ||
		((header_flags & QUERY_RESPONSE_MASK) != (uint16_t)RESPONSE_FLAG) ) {
		return false;
	}

	// usart_puts1("check error\n");

	// Check error
	if( (header_flags & TRUNCATION_FLAG) || (header_flags & RESP_MASK) ) {
		return false;
	}

	// usart_puts1("check answer\n");

	// And make sure we've got (at least) one answer
    memcpy(&staging, &header[6], sizeof(uint16_t));
	uint16_t answerCount = htons(staging);
	if (answerCount == 0 ) {
		return false;
	}
	
	// usart_puts1("skip question\n");

	// Skip over any questions
    memcpy(&staging, &header[4], sizeof(uint16_t));
	uint16_t i;
	for (uint16_t i =0; i < htons(staging); i++) {
		// Skip over the name
		uint8_t len;
		do{
			memcpy(&len, p, sizeof(len));
			p++;
			// sprintf(debug, "q len: %d\n", len);
			// usart_puts1(debug);
			p += len;
		}while(len != 0);

		// Now jump over the type and class
		p += 4;
		// p++;
	}

	// answer
	for(i=0;i< answerCount; i++){
		// Skip the name
		uint8_t len;
		do{
			memcpy(&len, p, sizeof(len));
			p++;
			if ((len & LABEL_COMPRESSION_MASK) == 0) {
				if(len > 0){
					// usart_puts1("len > 0\n");
					p += len;
				}
			}else{
				p++;
				len = 0;
			}
		}while(len  != 0);

        // Check the type and class
		uint16_t answerType;
        memcpy(&answerType, p, sizeof(answerType));
        p += sizeof(answerType);

		uint16_t answerClass;
        memcpy(&answerClass, p, sizeof(answerClass));
        p += sizeof(answerClass);

        // Ignore the Time-To-Live as we don't do any caching
        p += TTL_SIZE;

        memcpy(&header_flags, p, sizeof(header_flags));
        p += sizeof(header_flags);

        if ( (htons(answerType) == TYPE_A) && (htons(answerClass) == CLASS_IN) ) {
			if (htons(header_flags) != 4) {
                return false;
            }

            uint8_t address[4];
            memcpy(address, p, 4);
            
			sprintf(result, "%u.%u.%u.%u", address[0], address[1],address[2],address[3]);
            
			return true;
        }
    }

	return false;
}
#endif