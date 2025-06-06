#include <common.h>
#include "syscall.h"

void do_syscall(Context *c);
Context* schedule(Context *prev);

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD: {
      return schedule(c);
      /* should not reach here */
      while (1) {};
    }
    case EVENT_SYSCALL: {
      do_syscall(c);
      break;
    }
    case EVENT_IRQ_TIMER: {
      return schedule(c);
      /* should not reach here */
      while (1) {};
    }
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
