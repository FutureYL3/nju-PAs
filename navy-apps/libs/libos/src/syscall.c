#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <assert.h>
#include <time.h>
#include "syscall.h"
#include <stdio.h>
#include <NDL.h>
#include <errno.h>

// helper macros
#define _concat(x, y) x ## y
#define concat(x, y) _concat(x, y)
#define _args(n, list) concat(_arg, n) list
#define _arg0(a0, ...) a0
#define _arg1(a0, a1, ...) a1
#define _arg2(a0, a1, a2, ...) a2
#define _arg3(a0, a1, a2, a3, ...) a3
#define _arg4(a0, a1, a2, a3, a4, ...) a4
#define _arg5(a0, a1, a2, a3, a4, a5, ...) a5

// extract an argument from the macro array
#define SYSCALL  _args(0, ARGS_ARRAY)
#define GPR1 _args(1, ARGS_ARRAY)
#define GPR2 _args(2, ARGS_ARRAY)
#define GPR3 _args(3, ARGS_ARRAY)
#define GPR4 _args(4, ARGS_ARRAY)
#define GPRx _args(5, ARGS_ARRAY)

// ISA-depedent definitions
#if defined(__ISA_X86__)
# define ARGS_ARRAY ("int $0x80", "eax", "ebx", "ecx", "edx", "eax")
#elif defined(__ISA_MIPS32__)
# define ARGS_ARRAY ("syscall", "v0", "a0", "a1", "a2", "v0")
#elif defined(__ISA_RISCV32__) || defined(__ISA_RISCV64__)
# define ARGS_ARRAY ("ecall", "a7", "a0", "a1", "a2", "a0")
#elif defined(__ISA_AM_NATIVE__)
# define ARGS_ARRAY ("call *0x100000", "rdi", "rsi", "rdx", "rcx", "rax")
#elif defined(__ISA_X86_64__)
# define ARGS_ARRAY ("int $0x80", "rdi", "rsi", "rdx", "rcx", "rax")
#elif defined(__ISA_LOONGARCH32R__)
# define ARGS_ARRAY ("syscall 0", "a7", "a0", "a1", "a2", "a0")
#else
#error _syscall_ is not implemented
#endif

intptr_t _syscall_(intptr_t type, intptr_t a0, intptr_t a1, intptr_t a2) {
  register intptr_t _gpr1 asm (GPR1) = type;
  register intptr_t _gpr2 asm (GPR2) = a0;
  register intptr_t _gpr3 asm (GPR3) = a1;
  register intptr_t _gpr4 asm (GPR4) = a2;
  register intptr_t ret asm (GPRx);
  asm volatile (SYSCALL : "=r" (ret) : "r"(_gpr1), "r"(_gpr2), "r"(_gpr3), "r"(_gpr4));
  return ret;
}

#define MENU_PATH "/bin/menu"
#define NTERM_PATH "/bin/nterm"
#define START_PROGRAM NTERM_PATH

void _exit(int status) {
  // printf("here");
  // _syscall_(SYS_exit, status, 0, 0);
  // printf("exit with status %d, now jump to nterm\n", status);
  /* we make argv and envp both NULL to tell the kernel we need to reload nterm */
  /* normally, a process cannot invoke sys_execve with argv and envp both NULL */
  _syscall_(SYS_execve, (intptr_t) START_PROGRAM, (intptr_t) NULL, (intptr_t) NULL);
  
  /* should not reach here */
  while (1);
}

int _open(const char *path, int flags, mode_t mode) {
  return (int) _syscall_(SYS_open, (intptr_t) path, flags, mode);
}

int _write(int fd, void *buf, size_t count) {
  return (int) _syscall_(SYS_write, fd, (intptr_t) buf, count);
}

void *_sbrk(intptr_t increment) {
  extern char end;
  static char *addr = (char *) &end;

  void *old_addr = (void *) addr, *new_addr = (void *) (addr + increment);
  int ret = _syscall_(SYS_brk, (intptr_t) new_addr, 0, 0);
  if (ret < 0)  return (void *) -1;

  addr = new_addr;
  return old_addr;
}

int _read(int fd, void *buf, size_t count) {
  return (int) _syscall_(SYS_read, fd, (intptr_t) buf, count);
}

int _close(int fd) {
  return (int) _syscall_(SYS_close, fd, 0, 0);
}

off_t _lseek(int fd, off_t offset, int whence) {
  return (off_t) _syscall_(SYS_lseek, fd, offset, whence);
}

int _gettimeofday(struct timeval *tv, struct timezone *tz) {
  return _syscall_(SYS_gettimeofday, (intptr_t) tv, (intptr_t) tz, 0);
}

int _execve(const char *fname, char * const argv[], char *const envp[]) {
  // printf("%s\n", fname);
  // printf("%s\n", argv[1]);
  // printf("%s\n", *envp);
  // printf("here");
  int ret = _syscall_(SYS_execve, (intptr_t) fname, (intptr_t) argv, (intptr_t) envp);
  if (ret < 0) {
    errno = -ret;
    return -1;
  }
}

// Syscalls below are not used in Nanos-lite.
// But to pass linking, they are defined as dummy functions.

int _fstat(int fd, struct stat *buf) {
  return -1;
}

int _stat(const char *fname, struct stat *buf) {
  assert(0);
  return -1;
}

int _kill(int pid, int sig) {
  _exit(-SYS_kill);
  return -1;
}

pid_t _getpid() {
  _exit(-SYS_getpid);
  return 1;
}

pid_t _fork() {
  assert(0);
  return -1;
}

pid_t vfork() {
  assert(0);
  return -1;
}

int _link(const char *d, const char *n) {
  assert(0);
  return -1;
}

int _unlink(const char *n) {
  assert(0);
  return -1;
}

pid_t _wait(int *status) {
  assert(0);
  return -1;
}

clock_t _times(void *buf) {
  assert(0);
  return 0;
}

int pipe(int pipefd[2]) {
  assert(0);
  return -1;
}

int dup(int oldfd) {
  assert(0);
  return -1;
}

int dup2(int oldfd, int newfd) {
  return -1;
}

unsigned int sleep(unsigned int seconds) {
  assert(0);
  return -1;
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
  assert(0);
  return -1;
}

int symlink(const char *target, const char *linkpath) {
  assert(0);
  return -1;
}

int ioctl(int fd, unsigned long request, ...) {
  return -1;
}
