#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

void parse_hexstring(uint8_t* data, uint16_t length){
	uint8_t q=0;
	uint8_t p=0;
	uint16_t raw_length = 2*length;
	int i=0;
	int j=0;
	// char debug[100];
	
	while(i<raw_length){
		// sprintf(debug,"data[i]: %d\n", data[i]);
		// usart_puts1(debug);
		q = (data[i] >= 'A' && data[i]<='F')?(data[i]-'A'+10):((data[i]>='0' && data[i]<='9')?(data[i]-'0'):0);
		// sprintf(debug,"q1: %d\n", q);
		// usart_puts1(debug);
		i++;
		// sprintf(debug,"data[i]2: %d\n", data[i]);
		// usart_puts1(debug);
		p = (data[i] >= 'A' && data[i]<='F')?(data[i]-'A'+10):((data[i]>='0' && data[i]<='9')?(data[i]-'0'):0);
		// sprintf(debug,"p: %d\n", p);
		// usart_puts1(debug);

		q = 16*q + p;
		// sprintf(debug,"q2: %d\n", q);
		// usart_puts1(debug);
		data[j] = q;
		// sprintf(debug,"data[j]: %d\n", data[j]);
		// usart_puts1(debug);
        printf("parse: %02X\n", data[j]);
		j++;
		i++;
	}
}

bool DNSDecode(uint16_t requestID, uint8_t* packet, uint16_t packet_length, char* result){
    char debug[200];
	uint8_t header[DNS_HEADER_SIZE];
	uint8_t* p;
	p = packet;
    
    for(int i=0;i<packet_length;i++){
        printf("data[%d]: %02X\n", i, packet[i]);
    }
    
    memcpy(header, p, DNS_HEADER_SIZE);
	p += DNS_HEADER_SIZE;

	uint16_t staging;
	memcpy(&staging, &header[2], sizeof(uint16_t));
	uint16_t header_flags = htons(staging);
	memcpy(&staging, &header[0], sizeof(uint16_t));
	// Check requestID
	printf("check requestID\n");

    if ( ( requestID != staging ) ||
		((header_flags & QUERY_RESPONSE_MASK) != (uint16_t)RESPONSE_FLAG) ) {
		return false;
	}

	printf("check error\n");

	// Check error
	if( (header_flags & TRUNCATION_FLAG) || (header_flags & RESP_MASK) ) {
		return false;
	}

	// printf("check answer\n");

	// And make sure we've got (at least) one answer
    memcpy(&staging, &header[6], sizeof(uint16_t));
	uint16_t answerCount = htons(staging);
	if (answerCount == 0 ) {
		return false;
	}
	
	// printf("skip question\n");

	// Skip over any questions
    memcpy(&staging, &header[4], sizeof(uint16_t));
	uint16_t i;
	for (uint16_t i =0; i < htons(staging); i++) {
		// Skip over the name
		uint8_t len;
		do{
			memcpy(&len, p, sizeof(len));
            p++;
			// printf("q len: %d\n", len);
            for(int i=0;i<len;i++){
                printf("p: %02X\n", *p);
			    p++;
            }
		}while(len != 0);

		// Now jump over the type and class
		p += 4;
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

        // printf("break do while\n");

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
                // printf("weird size\n");
                return false;
            }

            // printf("get address\n");
            uint8_t address[4];
            memcpy(address, p, 4);
            sprintf(result, "%u.%u.%u.%u", address[0], address[1],address[2],address[3]);
            return true;
        }
    }
}

int main(){
    char data[] = "2DF48180000100010000000004636F6170066E657470696502696F0000010001C00C00010001000000260004CB96254C";
    uint16_t length = strlen(data)/2;
    printf("length: %u\n", length);

    parse_hexstring((uint8_t*)data, length);

    printf("length: %u\n", length);

    char result[25];
    bool ret = DNSDecode(62509,data, length, result);
    if(ret){
        printf("result: %s\n", result);
    }
    return 0;
}