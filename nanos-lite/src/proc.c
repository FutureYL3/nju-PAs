#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void naive_uload(PCB *pcb, const char *filename);

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%s' for the %dth time!", (char *) arg, j);
    j ++;
    yield();
  }
}

void context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  Area kstack = { .start = (void *) pcb->stack, .end = (void *) (pcb->stack + STACK_SIZE) };
  Context *context = kcontext(kstack, entry, arg);
  pcb->cp = context;
}

void init_proc() {
  /* create context for kernal thread `hello_fun`, and bind it to pcb[0] */
  context_kload(&pcb[0], hello_fun, "hello_fun 1");
  context_kload(&pcb[1], hello_fun, "hello_fun 2");
  switch_boot_pcb();

  // Log("Initializing processes...");

  // // load program here
	// naive_uload(NULL, "/bin/nterm");
}

Context* schedule(Context *prev) {
  // save the current context pointer
  current->cp = prev;

  /* select next process to run */
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);

  // then return the new context
  return current->cp;
}
