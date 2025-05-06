#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

void __libc_init_array(void);

int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  // unsigned int sp;
  // asm ("mv %0, sp" : "=r" (sp));
  // printf("current sp: 0x%x\n", sp);
  // uintptr_t *p = (uintptr_t *) args;
  int argc = *args;
  char **argv = (char **) (++args);
  char **envp = (char **) (args + argc + 1);
  environ = envp;
  /* call __libc_init_array to initialize cpp global object */
  __libc_init_array();
  
  exit(main(argc, argv, envp));
  assert(0);
}
