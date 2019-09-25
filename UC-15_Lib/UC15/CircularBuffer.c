#include "CircularBuffer.h"

/* Initialize and reset */
void circular_buf_reset(circular_buf_t* cbuf){
    cbuf->head = 0;
    cbuf->tail = 0;
    cbuf->full = false;
}

void circular_buf_init(circular_buf_t* cbuf){
    if(cbuf == NULL){
        return;
    }
    memset(cbuf->buffer, 0, CBUF_SIZE);
    cbuf->max = CBUF_SIZE;
    circular_buf_reset(cbuf);
    return;
}

/* state checks */
bool circular_buf_full(circular_buf_t* cbuf){
    return cbuf->full;
}

bool circular_buf_empty(circular_buf_t* cbuf){
    return (!cbuf->full && (cbuf->head == cbuf->tail));
}

size_t circular_buf_capacity(circular_buf_t* cbuf){
    return cbuf->max;
}

size_t circular_buf_size(circular_buf_t* cbuf){
    size_t size = cbuf->max;

    if(!cbuf->full){
        if(cbuf->head >= cbuf->tail){
            size = (cbuf->head - cbuf->tail);
        }else{
            size = (cbuf->max + cbuf->head - cbuf->tail);
        }
    }
    return size;
}

static void advance_pointer(circular_buf_t* cbuf){
    if(cbuf->full){
        cbuf->tail = (cbuf->tail + 1) % cbuf->max;
    }

    cbuf->head = (cbuf->head+1) % cbuf->max;
    cbuf->full = (cbuf->head == cbuf->tail);
}

static void retreat_pointer(circular_buf_t*  cbuf){
    cbuf->full = false;
    cbuf->tail = (cbuf->tail + 1) % cbuf->max;
}

void circular_buf_put(circular_buf_t* cbuf, uint8_t data){
    cbuf->buffer[cbuf->head] = data;
    advance_pointer(cbuf);
}

int circular_buf_get(circular_buf_t* cbuf, uint8_t* data){
    int ret = -1;
    if(!circular_buf_empty(cbuf)){
        *data = cbuf->buffer[cbuf->tail];
        retreat_pointer(cbuf);
        ret = 0;
    }
    return ret;
}