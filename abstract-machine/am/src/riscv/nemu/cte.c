#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    /* event packaging */
    switch (c->mcause) {
      case -1: { // yield
        ev.event = EVENT_YIELD;
        break;
      }
      case 1: { // syscall
        ev.event = EVENT_SYSCALL;
        break;
      }
      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  c->mepc += 4; // add 4 to mepc to avoid infinite ecall
  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  /* create the context */
  Context *context = (Context *) ((char *) kstack.end - sizeof(Context));
  /* set the kernal thread entry */
  context->mepc = (uintptr_t) entry - 4; // cooperate with `c->mepc += 4;`
  /* difftest */
  context->mstatus = 0x1800; // to pass difftest
  /* set arguments passed to the kernal thread */
  context->GPR2 = (uintptr_t) arg;
  return context;
}

void yield() {
  asm volatile("li a7, -1; ecall");
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
