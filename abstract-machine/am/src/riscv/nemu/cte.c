#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

void __am_get_cur_as(Context *c);
void __am_switch(Context *c);

#define IRQ_TIMER 0x80000007  // for riscv32

Context* __am_irq_handle(Context *c) {
  /* save current satp value to the context that is to be switched */
  __am_get_cur_as(c);
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
      case IRQ_TIMER: {
        printf("Got EVENT_IRQ_TIMER in am\n");
        ev.event = EVENT_IRQ_TIMER;
        break;
      }
      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  c->mepc += 4; // add 4 to mepc to avoid infinite ecall
  /* switch addr space, no switch needed for kernal thread */
  if (c->pdir != NULL)  __am_switch(c);
  printf("handle EVENT_IRQ_TIMER in am finished\n");
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

#define MIE 0x08

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  /* create the context */
  Context *context = (Context *) ((char *) kstack.end - sizeof(Context));
  /* set the kernal thread entry */
  context->mepc = (uintptr_t) entry - 4; // cooperate with `c->mepc += 4;`
  /* difftest */
  context->mstatus = 0x1800; // to pass difftest
  /* open interrupt */
  context->mstatus |= MIE;
  /* set arguments passed to the kernal thread */
  context->GPR2 = (uintptr_t) arg;
  /* set addr space pointer to NULL because every addr space am created has kernal map */
  context->pdir = NULL;

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
