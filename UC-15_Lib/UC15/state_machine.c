#include "state_machine.h"

void run_state( state_t* i ) {
    i->function(i);
};

void next_state(state_t *i, state_func_t f){
    i->function = f;
}