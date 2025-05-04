#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void naive_uload(PCB *pcb, const char *filename);
void context_kload(PCB *pcb, void (*entry)(void *), void *arg);
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);

void switch_boot_pcb() {
  current = &pcb_boot;
}

/* simulate kernel thread */
void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%s' for the %dth time!", (char *) arg, j);
    j ++;
    // yield();
  }
}

void init_proc() {
  Log("Initializing processes...");

  // load program here
  context_kload(&pcb[0], hello_fun, "hello_fun 1");
  // char *const argv1[] = {"/bin/hello", NULL};
  // char *const envp1[] = {NULL};
  // context_uload(&pcb[0], "/bin/hello", argv1, envp1);

  char *const argv2[] = {"/bin/nterm", NULL};
  char *const envp2[] = {NULL};
  context_uload(&pcb[1], "/bin/nterm", argv2, envp2);
  
  switch_boot_pcb();
  
	// naive_uload(NULL, "/bin/nterm");
}

Context* schedule(Context *prev) {
  static uint32_t helper = 0;
  // save the current context pointer
  current->cp = prev;

  /* select next process to run */
  if (helper++ % 10 == 0) {
    current = &pcb[0];
  }
  else {
    current = &pcb[1];
  }
  // current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);

  // then return the new context
  return current->cp;
}
