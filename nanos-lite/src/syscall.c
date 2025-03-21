#include <common.h>
#include "syscall.h"


void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

  // char *name = NULL;

  switch (a[0]) {
    case SYS_yield: {
      yield();
      c->GPRx = 0;
      // name = "SYS_yield";
      break;
    }
    case SYS_exit: {
      halt(a[1]);
      // name = "SYS_exit";
      break;
    }
    case SYS_write: {
      c->GPRx = write(a[1], (void *) a[2], a[3]);
      // name = "SYS_write";
      break;
    }
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

#if STRACE
  // if (name)  Log("syscall %s, with params a0=%d, a1=%d, a2=%d and ret=%d\n", name, a[1], a[2], a[3], c->GPRx);
#endif
}

ssize_t write(int fd, const void *buf, size_t count) {
  ssize_t written = 0;
  switch (fd) {
    /* for stdout and stderr */
    case 1: case 2: {
      char *p = (char *) buf;
      for (int i = 0; i < count; ++ i)  {
        putch(p[i]);
        ++written;
      }
      break;
    }
  
    default: {
      Log("error occur in syscall write for unknown fd: %d", fd);
      return -1;
    }
  }

  return written;
}
