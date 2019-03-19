#include "BC95.h"
#include "BC95Udp.h"

uint8_t BC95Udp_init(BC95UDP* bc95udp, BC95_str* bc95){
    if(bc95 == NULL){
        return 0;
    }
    bc95udp->bc95 = bc95;
    bc95udp->pbufferlen = 0;
    return 1;
}

uint8_t BC95Udp_begin(BC95UDP* bc95udp, uint16_t port){
    if(bc95udp->bc95 == NULL){
        return 0;
    }
	bc95udp->socket = BC95_createSocket(bc95udp->bc95,port);
    if(bc95udp->socket == NULL){
        return 0;
    }
	return 1;
}

void BC95Udp_stop(BC95UDP* bc95udp){
	BC95_closeSocket(bc95udp->bc95, bc95udp->socket);
}

// TODO DNS_beginPacket

int BC95Udp_beginPacket(BC95UDP* bc95udp, char *ip, uint16_t port){
	bc95udp->dip = ip;
    char cmd[20];
    //sprintf(cmd,"dip: %s\r\n", bc95udp->dip);
    sprintf(cmd, "dip is %d.%d.%d.%d\r\n", bc95udp->dip[0], bc95udp->dip[1], bc95udp->dip[2], bc95udp->dip[3]);
    BC95_usart_puts(cmd, USART2);
	bc95udp->dport = port;
	return 1;
}

size_t BC95Udp_write(BC95UDP* bc95udp, const uint8_t* buffer, size_t size) {
    //char cmd[20];
    //sprintf(cmd, "write dip is %d.%d.%d.%d\r\n", bc95udp->dip[0], bc95udp->dip[1], bc95udp->dip[2], bc95udp->dip[3]);
    //BC95_usart_puts(cmd, USART2);

	if (bc95udp->pbufferlen + size <= pbuffersize) {
        memcpy(bc95udp->pbuffer+bc95udp->pbufferlen, buffer, size);
        bc95udp->pbufferlen += size;
        return size;
    }    
    else {
        return 0;
    }
}

char* BC95Udp_endPacket(BC95UDP* bc95udp){
	char* result;
    char cmd[100];
	//sprintf(cmd, "try sendPacket..\r\n");
    //BC95_usart_puts(cmd, USART2);

    //if (bc95udp->bc95 != NULL) {
    //     sprintf(cmd, "bc95 OK\r\n");
    //     BC95_usart_puts(cmd, USART2);
    // }
    // if (bc95udp->socket != NULL) {
    //     sprintf(cmd, "socket OK\r\n");
    //     BC95_usart_puts(cmd, USART2);
    // }
    // if (bc95udp->dip != NULL) {
    //     sprintf(cmd, "IP len: %d\r\n", strlen(bc95udp->dip));
    //     BC95_usart_puts(cmd, USART2);

    //     sprintf(cmd, "dip OK\r\n");
    //     BC95_usart_puts(cmd, USART2);
    // }
    // if (bc95udp->dport != 0) {
    //     sprintf(cmd, "dport OK\r\n");
    //     BC95_usart_puts(cmd, USART2);
    // }
    
    // sprintf(cmd, "pbufferlen: %d\r\n", bc95udp->pbufferlen);
    // BC95_usart_puts(cmd, USART2);
    //bc95udp->dip[4] = '\0';
    // sprintf(cmd, "udp dip: %s\r\n", bc95udp->dip);
    // BC95_usart_puts(cmd, USART2);

    result = BC95_sendPacket(bc95udp->bc95, bc95udp->socket, bc95udp->dip, bc95udp->dport, bc95udp->pbuffer, bc95udp->pbufferlen);
    
    // sprintf(cmd, "try flush..\r\n");
    // BC95_usart_puts(cmd, USART2);

    BC95UDP_flush(bc95udp);

    sprintf(cmd, "endPacket done..\r\n");
    BC95_usart_puts(cmd, USART2);

    return result;
}

int BC95Udp_parsePacket(BC95UDP* bc95udp) {
	uint8_t *p,*q, *k, field;
    uint8_t i;
    size_t len;
    if (bc95udp->pbufferlen > 0) return bc95udp->pbufferlen;

    bc95udp->pbufferlen = 0;
    memset(bc95udp->pbuffer, 0, pbuffersize);
    q = bc95udp->pbuffer;

    int cnt = 0;
    //char cmd[10];
    do {
        cnt++;
        len = 0;

        char *msg = BC95_fetchSocketPacket(bc95udp->bc95,bc95udp->socket, BC95UDP_SERIAL_READ_CHUNK_SIZE);
        //BC95_usart_puts(msg, USART2);
        //
        //sprintf(cmd, ", %d\r\n",cnt);
        //BC95_usart_puts(cmd, USART2);

        if (msg == NULL) {
            //char cmd[10];
            //sprintf(cmd, "fetch NULL\r\n");
            //BC95_usart_puts(cmd, USART2);
            break;
        }
        p = (uint8_t*)msg;
        field = 0;
        while (field<4) {
            if (*p == ',') {
                field++;
                if (field==3) k = p+1;   // point to the beginning of returned size
                else if (field == 4) {
                    *p = '\0';
                    len = atoi((char *)k);
                    // shift p and break to avoid terminator checking below
                    p++;
                    break;
                }
            }

            if (*p=='\x00' || *p=='\x0D' || *p=='\x0A') {
                *(bc95udp->pbuffer) = '\0';
                break;
            }
            p++;
        }
        while (*p != '\0' && *p != ',') {
            *q = 0;
            for (i=0,*q=0; i<2; i++,p++) {
                *q = 16*(*q) + ((*p>='A' && *p<='F')?(*p-'A'+10):((*p>='0' && *p<='9')?(*p-'0'):0));
            }
            bc95udp->pbufferlen++;

            //sprintf(cmd, "%d\r\n",bc95udp->pbufferlen);
            //BC95_usart_puts(cmd, USART2);
            q++;
        }
        if (len < bc95udp->socket->bc95_msglen) bc95udp->socket->bc95_msglen -= len;
        else  bc95udp->socket->bc95_msglen = 0;

    } while (bc95udp->socket->bc95_msglen > 0);

    //sprintf(cmd, "parsePacket done..\r\n");
    //BC95_usart_puts(cmd, USART2);

    bc95udp->socket->buff_msglen = bc95udp->pbufferlen;
    return bc95udp->pbufferlen;
}

int BC95UDP_available(BC95UDP* bc95udp) {
	return bc95udp->pbufferlen;
}

int BC95UDP_read_null(BC95UDP* bc95udp) {
	unsigned char c;
	BC95UDP_read(bc95udp, &c, 1);
	return (int)c;
}

int BC95UDP_read(BC95UDP* bc95udp, uint8_t* buffer, size_t len) {
	uint16_t rlen;
	rlen = len>bc95udp->pbufferlen?bc95udp->pbufferlen:len;

	memcpy(buffer, bc95udp->pbuffer, rlen);

    memmove(bc95udp->pbuffer, bc95udp->pbuffer+rlen, bc95udp->pbufferlen-rlen);
    bc95udp->pbufferlen -= rlen;
    return rlen;
}

int BC95UDP_peek(BC95UDP* bc95udp) {
	if (bc95udp->pbufferlen > 0) return (int)(bc95udp->pbuffer);
    else return -1;
}

void BC95UDP_flush(BC95UDP* bc95udp){
	if (bc95udp->pbufferlen > 0) {
        memset(bc95udp->pbuffer, 0, pbuffersize);
        bc95udp->pbufferlen = 0;
    }
}

char* BC95UDP_remoteIP(BC95UDP* bc95udp){
	return bc95udp->dip;
}

uint16_t BC95UDP_remotePort(BC95UDP* bc95udp){
	return bc95udp->dport;
}
