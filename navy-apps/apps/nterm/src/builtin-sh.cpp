#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <SDL.h>
#include <stdlib.h>
#include <string.h>

struct file {
  char *file_name;
  #ifdef __GNUC__
  __attribute__((unused))
  #endif
  int file_size;
  #ifdef __GNUC__
  __attribute__((unused))
  #endif
  int offset;
} file_list[] = {
  #include "/home/yl/ics2022/navy-apps/build/ramdisk.h"
};

#define NR_FILE (sizeof(file_list) / sizeof(file_list[0]))

extern char **environ;

char handle_key(SDL_Event *ev);

static void sh_printf(const char *format, ...) {
  static char buf[256] = {};
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, 256, format, ap);
  va_end(ap);
  term->write(buf, len);
}

static void sh_banner() {
  sh_printf("Built-in Shell in NTerm (NJU Terminal)\n\n");
}

static void sh_prompt() {
  sh_printf("sh> ");
}

static inline bool isspace(char c) {
  return c == ' ';
}

static void sh_handle_cmd(const char *cmd) {
  setenv("PATH", "/bin", 0);
  char *cmd_copy = strdup(cmd);
  if (!cmd_copy) return;
  
  char *p = cmd_copy;
  char *command = NULL;
  bool first_arg = true;
  
  // 跳过前导空格
  while (*p && isspace(*p))  p++;

  
  // 解析命令名
  command = p;
  while (*p && !isspace(*p))  p++;
  if (*p)  *p++ = '\0';

  // 删除换行符
  char *pp = p;
  while (pp != NULL) {
    if (*pp == '\n') {
      *pp = '\0';
      break;
    }
    ++pp;
  }
  
  if (strcmp(command, "echo") == 0) {
    // 处理echo命令的参数
    while (*p) {
      // 跳过参数之间的空格
      while (*p && isspace(*p))  p++;
      if (!*p)  break;
      
      char *arg_start = p;
      char *arg_end = p;
      
      // 处理带引号的参数
      if (*p == '"') {
        p++; // 跳过开始引号
        arg_start = p;
        
        // 查找结束引号
        while (*p && *p != '"')  p++;
        if (*p == '"') {
          arg_end = p;
          *p++ = '\0'; // 替换引号为字符串结束符
        }
      } else {
        // 处理普通参数（无引号）
        while (*p && !isspace(*p)) p++;
        arg_end = p;
        if (*p) *p++ = '\0';
      }
      
      // 打印参数，第一个参数前不加空格
      if (!first_arg) sh_printf(" ");
      sh_printf("%s", arg_start);
      first_arg = false;
    }
    sh_printf("\n"); // 只在所有参数后添加一个换行
  }
  else if (strcmp(command, "ls") == 0) {
    sh_printf("Available apps:\n");
    for (int i = 0; i < NR_FILE; ++ i) {
      char *name = file_list[i].file_name;
      if (strncmp(name, "/bin/", 5) == 0)  sh_printf("%s\n", name);
    }
  }
  else { // 执行app，先查找，若未找到，提示用户
    // sh_printf("now executing program %s\n", command);
    int i;
    for (i = 0; i < NR_FILE; ++ i) {
      char *name = file_list[i].file_name;
      name += 5; // 跳过 /bin/
      char **argv = (char **) malloc(10 * sizeof(char *)); // 10 arguments should be enough
      if (strcmp(name, command) == 0) {
        int count = 0;
        argv[count++] = file_list[i].file_name;
        while (*p) {
          // 跳过参数之间的空格
          while (*p && isspace(*p))  p++;
          if (!*p)  break;
          
          char *arg_start = p;
          char *arg_end = p;
          
          // 处理带引号的参数
          if (*p == '"') {
            p++; // 跳过开始引号
            arg_start = p;
            
            // 查找结束引号
            while (*p && *p != '"')  p++;
            if (*p == '"') {
              arg_end = p;
              *p++ = '\0'; // 替换引号为字符串结束符
            }
          } else {
            // 处理普通参数（无引号）
            while (*p && !isspace(*p)) p++;
            arg_end = p;
            if (*p) *p++ = '\0';
          }
          
          // 保存参数
          // printf("%s\n", arg_start);
          argv[count++] = arg_start;
        }
        argv[count] = NULL;
        for (int j = 0; j < count; ++ j)  printf("%s\n", argv[j]);
        execvp(command, argv);
        break;
      }
    }
    // execve(command, NULL, environ);
    // execvp(command, NULL);
    if (i == NR_FILE)  sh_printf("unknown command: %s\n", command);
  }
  
  free(cmd_copy);
}

void builtin_sh_run() {
  sh_banner();
  sh_prompt();

  while (1) {
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_KEYUP || ev.type == SDL_KEYDOWN) {
        const char *res = term->keypress(handle_key(&ev));
        if (res) {
          sh_handle_cmd(res);
          sh_prompt();
        }
      }
    }
    refresh_terminal();
  }
}
