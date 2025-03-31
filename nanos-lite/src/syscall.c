#include <common.h>
#include <fs.h>
#include "syscall.h"
#include <proc.h>


#define STRACE 0


typedef struct file {
  char *name;
  size_t size;
  size_t disk_offset;
} file;

struct timeval {
  long tv_sec;    
  long tv_usec;
};

struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};

int brk(void *addr);
int gettimeofday(struct timeval *tv, struct timezone *tz);
void naive_uload(PCB *pcb, const char *filename);

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
    case SYS_gettimeofday: {
      c->GPRx = gettimeofday((struct timeval *) a[1], a[2] == 0 ? 0 : (struct timezone *) a[2]);
      break;
    }
    case SYS_execve: {
      /* if naive_uload failed, it will panic, so we don't check its return value to determine whether SYS_execve should return -1 */
      char absolut_path[50] = {0};
      const char **env = (const char **) a[3];
      while (strcmp(*env, "PATH") != 0)  ++env;
      const char *path = *env;
      while (*path != '=')  ++path;
      ++path;
      memcpy(absolut_path, path, sizeof(path));
      strcat(absolut_path, "/"); strcat(absolut_path, (const char *) a[1]);
      naive_uload(NULL, absolut_path);
      break;
    }
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

#if STRACE
static file files[] = {
  [0] = {"stdin", 0, 0},
  [1] = {"stdout", 0, 0},
  [2] = {"stderr", 0, 0},
  [3] = {"/dev/serial", 0, 0},
#include "files.h"
};
  char *name = NULL;
  bool is_sfs_call = false;
  char *file_name = NULL;
  switch (a[0]) {
    case SYS_yield: { name = "SYS_yield"; break; }
    case SYS_exit: { name = "SYS_exit"; break; }
    case SYS_brk: { name = "SYS_brk"; break; }
    case SYS_open: { name = "SYS_open"; file_name = (char *) a[1]; is_sfs_call = true; break; }
    case SYS_read: { name = "SYS_read"; is_sfs_call = true; break; }
    case SYS_write: { name = "SYS_write"; is_sfs_call = true; break; }
    case SYS_lseek: { name = "SYS_lseek"; is_sfs_call = true; break; }
    case SYS_close: { name = "SYS_close"; is_sfs_call = true; break; }
    case SYS_gettimeofday: { name = "SYS_gettimeofday"; break; }
  }
  if (name && !is_sfs_call)  Log("syscall %s, with params a0=%d, a1=%d, a2=%d and ret=%d", name, a[1], a[2], a[3], c->GPRx);
  if (name && is_sfs_call) {
    if (!file_name) {
      int fd = a[1];
      file_name = files[fd].name;
    }
    Log("syscall %s on file %s, with params a0=%d, a1=%s, a2=%d and ret=%d", name, file_name, a[1], a[2], a[3], c->GPRx);
  }
#endif
}

int brk(void *addr) {
  return 0;
}
/*
  Because am only has timer implemented as giving out the uptime of the system, so in struct tv, 
  we will set tv_sec to uptime / 1000000, and tv_usec to uptime % 1000000.

  always treat tz as NULL because tz is deprecated, so no operation will be performed on tz 
*/
int gettimeofday(struct timeval *tv, struct timezone *tz) {
  uint64_t uptime = io_read(AM_TIMER_UPTIME).us;
  tv->tv_sec = (long) (uptime / 1000000);
  tv->tv_usec = (long) (uptime % 1000000);
  return 0;
}
