#include <stdio.h>
#include "CircularBuffer.h"
#include "ATparser.h"

circular_buf_t cbuf;
atparser_t parser;

int readFunc(uint8_t* data){
    // printf("<");
    return circular_buf_get(&cbuf, data);
}

int writeFunc(uint8_t* buffer, size_t size){
    for(int i=0;i<size;i++){
        // printf("put: %d\n", buffer[i]);
        circular_buf_put(&cbuf, buffer[i]);
    }
    return 0;
}

bool readableFunc(){
    return !circular_buf_empty(&cbuf);
}

void sleepFunc(int us){
    // printf(".");
    usleep(us);
}

bool incoming_byte = false;

void urc_nsonmi(){
    printf("found +NSONMI:\n");
    incoming_byte = true;
}

void urc_neul(){
    printf("found neul\n");
}

void example_imi(){
    atparser_send(&parser, "AT+CIMI");
    if(atparser_recv(&parser, "AT+CIMI")){
        printf("found AT+CIMI\n");
    }

    atparser_send(&parser, "\r\n460001357924680\r\n\r\nOK\r\n");
    char imi[15];
    if(atparser_recv(&parser,"%s\r\n",imi) && atparser_recv(&parser, "OK")){
        printf("receive imi\n");
        printf("imi: %s\n", imi);
    }
}

void example_signal_strength(){
    atparser_send(&parser, "AT+CSQ");
    if(atparser_recv(&parser, "AT+CSQ")){
        printf("found AT+CSQ\n");
    }

    atparser_send(&parser, "\r\n+CSQ:1,99\r\n\r\nOK\r\n");
    int rssi;
    int ber;
    if(atparser_recv(&parser, "+CSQ:%d,%d\r\n",&rssi, &ber) && atparser_recv(&parser, "OK")){
        printf("rssi: %d\n", 2*rssi-113);
        printf("ber: %d\n", ber);
    }
}

void example_attach_network(){
    //Set command
    atparser_send(&parser, "AT+CGATT=1");
    if(atparser_recv(&parser, "AT+CGATT=1")){
        printf("found AT+CGATT=1\n");
    }

    atparser_send(&parser, "\r\nOK\r\n");
    if(atparser_recv(&parser, "OK")){
        printf("Attach OK\n");
    }

    //Read command
    atparser_send(&parser, "AT+CGATT?");
    if(atparser_recv(&parser, "AT+CGATT?")){
        printf("found AT+CGATT?\n");
    }

    atparser_send(&parser, "\r\n+CGATT:1\r\n\r\nOK\r\n");
    int state;
    if(atparser_recv(&parser, "+CGATT:%d\r\n",&state)){
        printf("state: %d\n", state);
    }

    char status[10];
    if(atparser_recv(&parser, "%s\r\n",&status)){
        printf("status: %s\n", status);
        if(strcmp(status,"OK") == 0){
            printf("OK\n");
        }else{
            printf("ERROR\n");
        }
    }
}

void example_create_socket(){
    //send command
    uint16_t port=8080;
    atparser_send(&parser, "AT+NSOCR=DGRAM,17,%u,1", port);
    if(atparser_recv(&parser, "AT+NSOCR=%*s\r\n")){
        printf("found AT+NSOCR\n");
    }
    
    //receive response
    atparser_send(&parser, "\r\n1\r\n\r\nOK\r\n");
    int socket=-1;
    if(atparser_recv(&parser, "%d\r\n", &socket)){
        printf("socket: %d\n", socket);
    }

    char status[10];
    if(atparser_recv(&parser, "%s\r\n",&status)){
        printf("status: %s\n", status);
        if(strcmp(status,"OK") == 0){
            printf("OK\n");
        }else{
            printf("ERROR\n");
        }
    }
}

void example_close_socket(){
    //send command
    int socket=1;
    atparser_send(&parser, "AT+NSOCL=%d\r\n", socket);
    if(atparser_recv(&parser, "AT+NSOCL=%*s\r\n")){
        printf("found AT+NSOCL\n");
    }
    
    //receive response
    atparser_send(&parser, "\r\nOK\r\n");
    char status[10];
    if(atparser_recv(&parser, "%s\r\n",&status)){
        printf("status: %s\n", status);
        if(strcmp(status,"OK") == 0){
            printf("OK\n");
        }else{
            printf("ERROR\n");
        }
    }
}

void example_send_packet(){
    char data[] = "Hello world";
    int data_length = strlen(data);

    uint16_t id=0;
    char ip[] = "192.168.1.1";
    uint16_t port=8080;

    uint8_t command[300];
    sprintf(command, "AT+NSOST=%u,%s,%u,%d,", id, ip, port, data_length);
    atparser_write(&parser, command, strlen(command));

    for(int i=0;i<data_length;i++){
        sprintf(command,"%02X", data[i]);
        atparser_write(&parser, command, strlen(command));
    }

    atparser_write(&parser,"\r\n",2);

    char rec[100];
    if(atparser_recv(&parser, "AT+NSOST=%u,%[^,],%u,%d,%[^,]\r\n", &id, rec, &port, &data_length, command)){
        printf("found AT+NSOST=%s\n",rec);
        printf("data=%s\n",command);
    }
}

int main(){
    printf("ATparser example\n");

    //setup
    circular_buf_init(&cbuf);
    atparser_init(&parser, readFunc, writeFunc, readableFunc, sleepFunc);
    atparser_oob(&parser, "+NSONMI:", urc_nsonmi);
    atparser_oob(&parser, "Neul", urc_neul);

    //testing oob
    {
        char list[] = "Neul\r\nOK\r\n\r\n+NSONMI:0,4\r\n";
        // char list[] = "OK\r\n";
        int length = strlen(list);
        printf("length: %d\n", length);

        atparser_write(&parser, list, length);

        if(atparser_process_oob(&parser)){
            printf("oob processed\n");
        }

        char list2[] = "\r\n0,192.168.5.1,1024,2,ABAB,0\r\n\r\nOK\r\n";
        length = strlen(list2);
        atparser_write(&parser, list2, length);

        int socket = -1;
        char ip[16];
        int port = -1;
        int data_length = 0;
        uint8_t data[20];
        int remaining_length = 0;
        if(atparser_recv(&parser, "%d,%[^,],%d,%d,%[^,],%d\r\n", &socket, ip, &port, &data_length, data, &remaining_length) && atparser_recv(&parser, "OK")){
            printf("socket: %d\n", socket);
            printf("ip: %s\n", ip);
            printf("port: %d\n", port);
            printf("data_length: %d\n", data_length);
            printf("data: %s\n", data);
            printf("remaining_length: %d\n", remaining_length);
            printf("found OK\n");
        }
    }
    //IMI
    example_imi();

    //Signal strength
    example_signal_strength();

    //Attach network
    example_attach_network();

    //Create UDP socket
    example_create_socket();

    //Close UDP socket
    example_close_socket();

    //send UDP packet
    example_send_packet();
}