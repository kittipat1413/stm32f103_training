#include "ATparser.h"

void atparser_init(atparser_t* self, atparser_reader reader,atparser_writer writer, atparser_readableFunc readable, atparser_sleepFunc sleep){
    self->timeout = 8000000;
    self->_buffer_size = 512;
    
    self->read = reader;
    self->write = writer;
    self->readable = readable;
    self->sleep = sleep;

    self->interval = 1000;
    atparser_set_delimiter(self,"\r\n");
}

void atparser_set_timeout(atparser_t* self, int timeout){
    self->timeout = timeout;
}

void atparser_set_delimiter(atparser_t* self, const char *output_delimiter){
    self->_output_delimiter = output_delimiter;
    self->_outout_delim_size = strlen(output_delimiter);
}

int atparser_putc(atparser_t* self, char c){
    return self->write((uint8_t*)&c,1);
}

int atparser_getc(atparser_t* self){
    int count = self->timeout/self->interval;
    while(count>0){
        count--;
        uint8_t data;
        int ret = self->read(&data);
        if(ret != -1){
            return (int)data;
        }
        self->sleep(self->interval);
    }
    return -1;
}

void atparser_flush(atparser_t* self){
    uint8_t data;
    while(self->readable()){
        self->read(&data);
    }
}

int atparser_write(atparser_t* self, uint8_t* data, int size){
    int i=0;
    for(; i< size;i++){
        if(atparser_putc(self,data[i]) < 0){
            return -1;
        }
    }
    return i;
}

int atparser_read(atparser_t* self, uint8_t* data, int size){
    int i=0;
    for(;i< size;i++){
        int c = atparser_getc(self);
        if(c < 0){
            return -1;
        }
        data[i] = c;
    }
    return i;
}

bool atparser_vsend(atparser_t* self, const char *command, va_list args){
    // Create and send command
    if(vsprintf(self->_buffer, command, args) < 0){
        return false;
    }

    int i;
    for(i=0; self->_buffer[i]; i++){
        if(atparser_putc(self, self->_buffer[i]) < 0){
            return false;
        }
    }

    // Finish with newline
    size_t j;
    for(j = 0; self->_output_delimiter[j]; j++){
        if(atparser_putc(self, self->_output_delimiter[j]) < 0){
            return false;
        }
    }

    return true;
}

bool atparser_send(atparser_t* self, const char *command, ...){
    va_list args;
    va_start(args, command);
    bool res = atparser_vsend(self, command, args);
    va_end(args);
    return res;
}

bool atparser_vrecv(atparser_t* self, const char *response, va_list args){
restart:
    // Iterate through each line in the expected response
    // response being NULL means we just want to check for OOBs
    while(!response || response[0]){
        // Since response is const, we need to copy it into our buffer to
        // add the line's null terminator and clobber value-matches with asterisks.
        //
        // We just use the beginning of the buffer to avoid unnecessary allocations.
        int i=0;
        int offset = 0;
        bool whole_line_wanted = false;

        while (response && response[i]) {
            if (response[i] == '%' && response[i + 1] != '%' && response[i + 1] != '*') {
                self->_buffer[offset++] = '%';
                self->_buffer[offset++] = '*';
                i++;
            } else {
                self->_buffer[offset++] = response[i++];
                // Find linebreaks, taking care not to be fooled if they're in a %[^\n] conversion specification
                if (response[i - 1] == '\n' && !(i >= 3 && response[i - 3] == '[' && response[i - 2] == '^')) {
                    whole_line_wanted = true;
                    break;
                }
            }
        }
        // Scanf has very poor support for catching errors
        // fortunately, we can abuse the %n specifier to determine
        // if the entire string was matched.
        self->_buffer[offset++] = '%';
        self->_buffer[offset++] = 'n';
        self->_buffer[offset++] = 0;
        // To workaround scanf's lack of error reporting, we actually
        // make two passes. One checks the validity with the modified
        // format string that only stores the matched characters (%n).
        // The other reads in the actual matched values.
        //
        // We keep trying the match until we succeed or some other error
        // derails us.
        int j = 0;

        while (true) {
            // If just peeking for OOBs, and at start of line, check
            // readability
            if (!response && j == 0 && !self->readable()) {
                return false;
            }
            // Receive next character
            int c = atparser_getc(self);
            if (c < 0) {
                // debug_if(_dbg_on, "AT(Timeout)\n");
                return false;
            }
            // Simplify newlines (borrowed from retarget.cpp)
            if ((c == CR && self->_in_prev != LF) ||
                    (c == LF && self->_in_prev != CR)) {
                self->_in_prev = c;
                c = '\n';
            } else if ((c == CR && self->_in_prev == LF) ||
                       (c == LF && self->_in_prev == CR)) {
                self->_in_prev = c;
                // onto next character
                continue;
            } else {
                self->_in_prev = c;
            }
            self->_buffer[offset + j++] = c;
            self->_buffer[offset + j] = 0;

            // Check for oob data
            struct oob *oob;
            for (oob = self->_oobs; oob; oob = oob->next) {
                if ((unsigned)j == oob->len && memcmp(
                            oob->prefix, self->_buffer + offset, oob->len) == 0) {
                    // debug_if(_dbg_on, "AT! %s\n", oob->prefix);
                    self->_oob_cb_count++;
                    oob->cb();

                    if (self->_aborted) {
                        // debug_if(_dbg_on, "AT(Aborted)\n");
                        return false;
                    }
                    // oob may have corrupted non-reentrant buffer,
                    // so we need to set it up again
                    goto restart;
                }
            }

            // Check for match
            int count = -1;
            if (whole_line_wanted && c != '\n') {
                // Don't attempt scanning until we get delimiter if they included it in format
                // This allows recv("Foo: %s\n") to work, and not match with just the first character of a string
                // (scanf does not itself match whitespace in its format string, so \n is not significant to it)
            } else if (response) {
                sscanf(self->_buffer + offset, self->_buffer, &count);
            }

            // We only succeed if all characters in the response are matched
            if (count == j) {
                // debug_if(_dbg_on, "AT= %s\n", _buffer + offset);
                // Reuse the front end of the buffer
                memcpy(self->_buffer, response, i);
                self->_buffer[i] = 0;

                // Store the found results
                vsscanf(self->_buffer + offset, self->_buffer, args);

                // Jump to next line and continue parsing
                response += i;
                break;
            }

            // Clear the buffer when we hit a newline or ran out of space
            // running out of space usually means we ran into binary data
            if (c == '\n' || j + 1 >= self->_buffer_size - offset) {
                // debug_if(_dbg_on, "AT< %s", _buffer + offset);
                j = 0;
            }
        }
    }
    return true;
}

bool atparser_recv(atparser_t* self, const char *response, ...)
{
    va_list args;
    va_start(args, response);
    bool res = atparser_vrecv(self, response, args);
    va_end(args);
    return res;
}

// oob registration
void atparser_oob(atparser_t* self, const char *prefix, atparser_callback cb){
    oob *newoob = (oob*)malloc(sizeof(oob));
    newoob->len = strlen(prefix);
    newoob->prefix = prefix;
    newoob->cb = cb;
    newoob->next = self->_oobs;
    self->_oobs = newoob;
}

bool atparser_process_oob(atparser_t* self){
    int pre_count = self->_oob_cb_count;
    atparser_recv(self, NULL);
    return self->_oob_cb_count != pre_count;
}