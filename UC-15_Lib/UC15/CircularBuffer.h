#ifndef CIRCULARBUFFER_H_
#define CIRCULARBUFFER_H_
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define CBUF_SIZE 512

/* Guide */
/* https://embeddedartistry.com/blog/2017/4/6/circular-buffers-in-cc */
typedef struct circular_buf_t
{
    uint8_t buffer[CBUF_SIZE];
    size_t head;
    size_t tail;
    size_t max;
    bool full;
} circular_buf_t;

/* Initialize and reset */
void circular_buf_reset(circular_buf_t* cbuf);
void circular_buf_init(circular_buf_t* cbuf);

/* state checks */
bool circular_buf_full(circular_buf_t* cbuf);
bool circular_buf_empty(circular_buf_t* cbuf);
size_t circular_buf_capacity(circular_buf_t* cbuf);
size_t circular_buf_size(circular_buf_t* cbuf);
// static void advance_pointer(circular_buf_t* cbuf);
// static void retreat_pointer(circular_buf_t*  cbuf);
void circular_buf_put(circular_buf_t* cbuf, uint8_t data);
int circular_buf_get(circular_buf_t* cbuf, uint8_t* data);
#endif