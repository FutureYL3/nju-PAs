#include <common.h>
#include <fs.h>
#include "syscall.h"

int brk(void *addr);

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

  switch (a[0]) {
    case SYS_yield: {
      yield();
      c->GPRx = 0;
      break;
    }
    case SYS_exit: {
      halt(a[1]);
      break;
    }
    case SYS_open: {
      c->GPRx = fs_open((char *) a[1], a[2], a[3]);
      break;
    }
    case SYS_read: {
      c->GPRx = fs_read(a[1], (void *) a[2], a[3]);
      break;
    }
    case SYS_write: {
      c->GPRx = fs_write(a[1], (void *) a[2], a[3]);
      break;
    }
    case SYS_lseek: {
      c->GPRx = fs_lseek(a[1], a[2], a[3]);
      break;
    }
    case SYS_close: {
      c->GPRx = fs_close(a[1]);
      break;
    }
    case SYS_brk: {
      c->GPRx = brk((void *) a[1]);
      break;
    }
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

#if STRACE
  char *name = NULL;
  switch (a[0]) {
    case SYS_yield: { name = "SYS_yield"; break; }
    case SYS_exit: { name = "SYS_exit"; break; }
    case SYS_write: { name = "SYS_write"; break; }
    case SYS_brk: { name = "SYS_brk"; break; }
  }
  if (name)  Log("syscall %s, with params a0=%d, a1=%d, a2=%d and ret=%d", name, a[1], a[2], a[3], c->GPRx);
#endif
}

int brk(void *addr) {
  return 0;
}
