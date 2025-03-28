#include <stdlib.h>
#include <am.h>
#include <NDL.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

/* screen width and height */
static int w, h;

#define N 4

void __am_gpu_init() {
  int fd = open("/proc/dispinfo", 0, 0); // we do not use flags and mode
  char buf[64] = {0}; // 64 should be enough
  read(fd, buf, sizeof(buf));
  close(fd);
  /* 5 lines should be enough */
  char *lines[5];
  int num_lines = 0;
  char *line = strtok(buf, "\n");
  while (line != NULL && num_lines < 5) {
    lines[num_lines++] = line;
    line = strtok(NULL, "\n");
  }
  for (int i = 0; i < num_lines; i++) {
    char *key = strtok(lines[i], " :");
    char *value = strtok(NULL, " :");
    if (key && value) {
      if (strcmp(key, "WIDTH") == 0)          w = atoi(value);
      else if (strcmp(key, "HEIGHT") == 0)    h = atoi(value);
    }
  }
  close(fd);
  
  NDL_OpenCanvas(&w, &h);
}


void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = w, .height = h,
    .vmemsz = w * h * N
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  uint32_t *p = (uint32_t *) ctl->pixels;
  NDL_DrawRect(p, ctl->x, ctl->y, ctl->w, ctl->h);
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}