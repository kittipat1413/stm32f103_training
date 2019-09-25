#ifndef ATPARSER_H_
#define ATPARSER_H_

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef LF
#undef LF
#define LF  10
#else
#define LF  10
#endif

#ifdef CR
#undef CR
#define CR  13
#else
#define CR  13
#endif

typedef int (*atparser_reader)(uint8_t* data);
typedef int (*atparser_writer)(uint8_t* buffer, size_t size);
typedef void (*atparser_sleepFunc)(int us);
typedef bool (*atparser_readableFunc)();
typedef void (*atparser_callback)();

typedef struct oob oob;
struct oob {
    unsigned len;
    const char *prefix;
    atparser_callback cb;
    oob *next;
};

struct atparser_t
{    
    int timeout;
    int _buffer_size;
    char _buffer[512];    
    atparser_reader read; 
    atparser_writer write;

    atparser_readableFunc readable;
    atparser_sleepFunc sleep;

    const char *_output_delimiter;
    int _outout_delim_size;
    int _oob_cb_count;
    char _in_prev;
    bool _dbg_on;
    bool _aborted;

    oob *_oobs;

    int interval;
};// atparser_default = {1000000, 512};

typedef struct atparser_t atparser_t;

void atparser_init(atparser_t* self, atparser_reader reader,atparser_writer writer, atparser_readableFunc readable, atparser_sleepFunc sleep);
void atparser_set_timeout(atparser_t* self, int timeout);
void atparser_set_delimiter(atparser_t* self, const char *output_delimiter);

int atparser_putc(atparser_t* self, char c);

int atparser_getc(atparser_t* self);

void atparser_flush(atparser_t* self);

int atparser_write(atparser_t* self, uint8_t* data, int size);

int atparser_read(atparser_t* self, uint8_t* data, int size);

bool atparser_vsend(atparser_t* self, const char *command, va_list args);

bool atparser_send(atparser_t* self, const char *command, ...);

bool atparser_vrecv(atparser_t* self, const char *response, va_list args);

bool atparser_recv(atparser_t* self, const char *response, ...);

// oob registration
void atparser_oob(atparser_t* self, const char *prefix, atparser_callback cb);

bool atparser_process_oob(atparser_t* self);
#endif