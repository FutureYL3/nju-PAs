#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

void __libc_init_array(void);

int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  uint32_t *p = (uint32_t *) args;
  int argc = *p;
  char **argv = (char **) (++p);
  char **envp = (char **) (p + argc + 1);
  environ = envp;
  /* call __libc_init_array to initialize cpp global object */
  __libc_init_array();
  exit(main(argc, argv, envp));
  assert(0);
}
