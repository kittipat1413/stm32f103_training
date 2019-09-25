#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

struct state;
typedef void (*state_func_t)(struct state *);

typedef struct state
{
  state_func_t function;

  // other stateful data

} state_t;

void run_state(state_t *i);
void next_state(state_t *i, state_func_t f);
#endif