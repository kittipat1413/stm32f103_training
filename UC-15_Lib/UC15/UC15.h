#ifndef UC15_H_
#define UC15_H_
#include "ATparser.h"

const char no_carrier_string[10] = {'N', 'O', ' ', 'C', 'A', 'R', 'R', 'I', 'E', 'R'};

typedef struct UC15
{
    bool no_carrier_flag;
    int no_carrier_ptr;
    atparser_t *parser;

} UC15;

void UC15_init(UC15 *uc15, atparser_t *parser);
bool UC15_is_ready(UC15 *uc15);
bool UC15_activate(UC15 *uc15);
bool UC15_get_IP(UC15 *uc15, char *ip);
bool UC15_connect(UC15 *uc15, char *host, int port);
bool UC15_disconnect(UC15 *uc15);
bool UC15_close(UC15 *uc15);
bool UC15_deactivate(UC15 *uc15);
bool UC15_ping(UC15 *uc15, char *endpoint);
ssize_t UC15_read(UC15 *uc15, char *buffer, size_t length);
ssize_t UC15_write(UC15 *uc15, char *buffer, size_t length);

void UC15_init(UC15 *uc15, atparser_t *parser)
{   
    uc15->no_carrier_ptr = 0;
    uc15->no_carrier_flag = false;
    uc15->parser = parser;
}

bool UC15_is_ready(UC15 *uc15)
{
    atparser_set_timeout(uc15->parser, 30000000);
    if (atparser_recv(uc15->parser, "+QIND: PB DONE"))
    {
        atparser_set_timeout(uc15->parser, 8000000);
        return true;
    }
    atparser_set_timeout(uc15->parser, 8000000);
    return false;
}

bool UC15_activate(UC15 *uc15)
{
    atparser_send(uc15->parser, "AT+QIACT=1");
    uc15->parser->sleep(1000000);
    atparser_set_timeout(uc15->parser, 30000000);
    if (atparser_recv(uc15->parser, "OK"))
    {
        atparser_set_timeout(uc15->parser, 8000000);
        return true;
    }
    atparser_set_timeout(uc15->parser, 8000000);
    return false;
}

bool UC15_get_IP(UC15 *uc15, char *ip)
{
    atparser_send(uc15->parser, "AT+QIACT?");
    if (atparser_recv(uc15->parser, "+QIACT: %[^\n]%*c", ip) && atparser_recv(uc15->parser, "OK"))
    {
        return true;
    }
    return false;
}

bool UC15_connect(UC15 *uc15, char *host, int port)
{
    atparser_send(uc15->parser, "AT+QIOPEN=1,0,\"TCP\",\"%s\",%d,0,2", host, port);
    atparser_set_timeout(uc15->parser, 30000000);
    if (atparser_recv(uc15->parser, "CONNECT"))
    {
        atparser_set_timeout(uc15->parser, 8000000);
        atparser_flush(uc15->parser);
        return true;
    }
    atparser_set_timeout(uc15->parser, 8000000);
    return false;
}

bool UC15_ping(UC15 *uc15, char *endpoint){
    atparser_send(uc15->parser, "AT+QPING=1,\"%s\"", endpoint);
    if(atparser_recv(uc15->parser, "OK")){
        return true;
    }
    return false;
}

bool UC15_disconnect(UC15 *uc15)
{
    uc15->parser->sleep(2000000);
    char terminator[3];
    terminator[0] = '+';
    terminator[1] = '+';
    terminator[2] = '+';
    atparser_write(uc15->parser, (uint8_t *)terminator, 3);
}

bool UC15_close(UC15 *uc15)
{
    atparser_send(uc15->parser, "AT+QICLOSE=0");
    atparser_set_timeout(uc15->parser, 15000000);
    if (atparser_recv(uc15->parser, "OK"))
    {
        atparser_set_timeout(uc15->parser, 8000000);
        return true;
    }
    atparser_set_timeout(uc15->parser, 8000000);
    return false;
}

bool UC15_deactivate(UC15 *uc15)
{
    atparser_send(uc15->parser, "AT+QIDEACT=1");
    atparser_set_timeout(uc15->parser, 40000000);
    if (atparser_recv(uc15->parser, "OK"))
    {
        atparser_set_timeout(uc15->parser, 8000000);
        return true;
    }
    atparser_set_timeout(uc15->parser, 8000000);
    return false;
}

ssize_t UC15_read(UC15 *uc15, char *buffer, size_t length)
{
    int ret = atparser_read(uc15->parser, buffer, length);
    if (ret > 0)
    {
        for (int i = 0; i < ret; i++)
        {
            if (buffer[i] == no_carrier_string[uc15->no_carrier_ptr])
            {
                uc15->no_carrier_ptr++;
                if (uc15->no_carrier_ptr == 10)
                {
                    uc15->no_carrier_flag = true;
                    uc15->no_carrier_ptr = 0;
                }
            }
        }
    }
    else
    {
        return 0;
    }
    return ret;
}

ssize_t UC15_write(UC15 *uc15, char *buffer, size_t length)
{
    return atparser_write(uc15->parser, buffer, length);
}
#endif
