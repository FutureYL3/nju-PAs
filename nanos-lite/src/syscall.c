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
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);
void switch_boot_pcb();

extern PCB* current;

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
      // --- 从用户寄存器获取原始指针 ---
      const char *u_filename = (const char *) a[1];
      char *const *u_argv = (char *const *) a[2];
      char *const *u_envp = (char *const *) a[3];

      // check whether the file exist
      if (fs_open(u_filename, 0, 0) < 0) {
        c->GPRx = -2;
        break;
      }

      // --- 定义内核缓冲区和限制 ---
      #define K_MAX_ARGS 10      // 内核允许的最大参数数量 (包括 argv[0])
      #define K_MAX_ENVS 10      // 内核允许的最大环境变量数量
      #define K_STR_LEN 256      // filename, argv[*], envp[*] 的最大长度 (包括 '\0')

      char k_filename[K_STR_LEN] = {0}; // 缓冲区：存储文件名的副本
      char k_argv_storage[K_MAX_ARGS][K_STR_LEN] = {{0}}; // 缓冲区：存储所有 argv 字符串的副本
      char k_envp_storage[K_MAX_ENVS][K_STR_LEN] = {{0}}; // 缓冲区：存储所有 envp 字符串的副本

      // --- 内核空间的指针数组 (这才是要传递给 context_uload 的！) ---
      char *k_argv[K_MAX_ARGS + 1]; // +1 for NULL terminator
      char *k_envp[K_MAX_ENVS + 1]; // +1 for NULL terminator

      int argc = 0;
      int envc = 0;

      // --- (可选但推荐) 基本的指针有效性检查 ---
      if (!u_filename) {
        Log("Execve error: received NULL pointer for filename.");
        c->GPRx = -1; // 设置错误返回值
        break;        // 退出 syscall 处理
      }

      // --- 1. 复制 filename ---
      // !!! 仍然是风险点：直接访问用户内存 !!!
      size_t len = strlen(u_filename);
      if (len >= K_STR_LEN) { // 检查长度（>= 因为要算上 '\0'）
        Log("Execve error: filename too long (max %d).", K_STR_LEN - 1);
        c->GPRx = -1;
        break;
      }
      strcpy(k_filename, u_filename);

      // --- 检查是否是由 _exit 发起的特殊 "reload nterm" 情况 ---
      if (u_argv == NULL && u_envp == NULL && strcmp(k_filename, "/bin/nterm") == 0) {
          Log("Execve: 检测到来自 _exit 的 /bin/nterm 特殊重载请求。");
          // 内核内部生成 nterm 的默认参数/环境
          strcpy(k_argv_storage[0], "/bin/nterm");
          k_argv[0] = k_argv_storage[0];
          argc = 1;
          k_argv[argc] = NULL; // 结束 argv

          // 这种情况下不需要环境变量
          envc = 0;
          k_envp[envc] = NULL; // 结束 envp
      }
      else {
        // --- 正常的 execve 情况：从用户空间复制 argv 和 envp ---
        // (将之前复制参数/环境的逻辑放在这里)
        // 在继续之前检查 u_argv 或 u_envp 是否为 NULL
        if (!u_argv || !u_envp) {
          Log("Execve error: receive NULL argv and envp pointer in normal situation.");
          c->GPRx = -1; // 设置错误返回值
          break;        // 退出 syscall 处理
        }

        // --- 2. 复制 argv ---
        for (argc = 0; argc < K_MAX_ARGS; ++argc) {
          char *u_arg = u_argv[argc]; // 获取用户空间的 argv 指针
          if (u_arg == NULL) {
            break; // 用户 argv 结束
          }

          len = strlen(u_arg); // 获取用户参数长度 (风险点)
          if (len >= K_STR_LEN) {
            Log("Execve error: argument %d too long (max %d).", argc, K_STR_LEN - 1);
            c->GPRx = -1;
            break;
          }

          // 复制字符串到内核存储区 k_argv_storage[argc]
          strcpy(k_argv_storage[argc], u_arg);
          // 将指向内核存储区字符串的指针存入内核指针数组 k_argv[argc]
          k_argv[argc] = k_argv_storage[argc];
        }
        // 检查是否因为参数过多而退出循环
        if (argc == K_MAX_ARGS && u_argv[argc] != NULL) {
          Log("Execve error: too many arguments (limit %d).", K_MAX_ARGS);
          c->GPRx = -1;
          break;
        }
        k_argv[argc] = NULL; // 正确地用 NULL 终止内核指针数组

        // --- 3. 复制 envp (逻辑同 argv) ---
        for (envc = 0; envc < K_MAX_ENVS; ++envc) {
          char *u_env = u_envp[envc];
          if (u_env == NULL) {
            break;
          }

          len = strlen(u_env);
          if (len >= K_STR_LEN) {
            Log("Execve error: environment variable %d too long (max %d).", envc, K_STR_LEN - 1);
            c->GPRx = -1;
            break;
          }

          strcpy(k_envp_storage[envc], u_env);
          k_envp[envc] = k_envp_storage[envc];
        }
        if (envc == K_MAX_ENVS && u_envp[envc] != NULL) {
          Log("Execve error: too many environment variables (limit %d).", K_MAX_ENVS);
          c->GPRx = -1;
          break;
        }
        k_envp[envc] = NULL; // 正确地用 NULL 终止内核指针数组
      }


      // --- 4. 使用正确的内核指针数组调用 context_uload ---
      // 现在传递的是 k_argv (类型 char *[]) 和 k_envp (类型 char *[])，
      // 它们会正确地衰变为 context_uload 期望的 char ** 类型。
      context_uload(current, k_filename, k_argv, k_envp);

      // --- 5. context_uload 成功后 ---
      switch_boot_pcb();
      yield();

      // 正常情况不应到达这里
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
