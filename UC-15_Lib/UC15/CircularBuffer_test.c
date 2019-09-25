#include <stdio.h>
#include "CircularBuffer.h"

circular_buf_t cbuf;

int main(){
    printf("hello circular buffer\n");

    circular_buf_init(&cbuf);

    bool full = circular_buf_full(&cbuf);
    printf("isfull: %d\n", full);

    bool empty = circular_buf_empty(&cbuf);
    printf("isempty: %d\n", empty);

    char list[] = "123\r\nOK\r\n";
    int length = strlen(list);
    printf("length: %d\n", length);
    for(int i=0;i<length;i++){
        circular_buf_put(&cbuf, list[i]);
    }

    uint8_t data;
    while(circular_buf_get(&cbuf, &data) != -1){
        printf("data: %c\n", data);
    }
}