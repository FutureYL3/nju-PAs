#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;
static int canvas_w = 0, canvas_h = 0;

uint32_t NDL_GetTicks() {
  struct timeval tv = {};
  int ret = gettimeofday(&tv, NULL);
  if (ret < 0) {
    return -1;
  }
  // ms as unit
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int NDL_PollEvent(char *buf, int len) {
  /* in sfs, we do not use flags and mode */
  int fd = open("/dev/events", 0, 0);
  int ret = read(fd, (void *) buf, len);
  if (ret == 0)  return 0;

  return 1;
}

/* currently, we set canvas to the left-top corner of the screen */
void NDL_OpenCanvas(int *w, int *h) {
  if (*w > screen_w || *h > screen_h) {
    fprintf(stderr, "Canvas width/height exceeds screen width/height\n");
    return;
  }
  if (*w == 0 && *h == 0) {
    *w = screen_w;
    *h = screen_h;
  }

  canvas_w = *w;
  canvas_h = *h;

  // printf("The screen size: width %d, height %d\n", screen_w, screen_h);
  // printf("The canvas size: width %d, height %d\n", *w, *h);
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
}

int fd = -1;
/* this function's target is canvas, not screen */
void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  if (!pixels || w <= 0 || h <= 0)  return;
  // check for border
  if (x < 0 || y < 0 || x + w > canvas_w || y + h > canvas_h) {
    return;
  }
  /* in sfs, flags and mode are not used */
  if (fd == -1)  fd = open("/dev/fb", 0, 0);  
  uint32_t offset = (screen_w * y + x) * 4;
  uint32_t *p = pixels;
  lseek(fd, offset, SEEK_SET);
  /* we use upper 16 bits for w and lower 16 bits for h */
  uint32_t len = (uint16_t) h + (w << 16);
  // printf("in NDL_DrawRect, w is %d and h = %d\n", w, h);
  // printf("in NDL_DrawRect, len is %d\n", len);
  // for (int i = 0; i < h; ++ i) {
  //   write(fd, (void *) p, w * 4);
  //   offset += screen_w * 4;
  //   p += w;
  //   lseek(fd, offset, SEEK_SET);
  // }
  write(fd, (void *) p, len);
}

void NDL_OpenAudio(int freq, int channels, int samples) {
  /* in sfs, flags and mode are not used */
  int fd = open("/dev/sbctl", 0, 0);
  int buf[] = { freq, channels, samples };
  write(fd, buf, sizeof(buf));
  close(fd);
}

int sb_fd = -1;
int NDL_PlayAudio(void *buf, int len) {
  if (sb_fd == -1)  sb_fd = open("/dev/sb", 0, 0);

  return write(sb_fd, buf, len);
}

int sbsize_fd = -1;
int NDL_QueryAudio() {
  if (sbsize_fd == -1)  sbsize_fd = open("/dev/sbctl", 0, 0);

  return read(sbsize_fd, NULL, 0);
}

void NDL_CloseAudio() {
  close(sb_fd);
  close(sbsize_fd);
  sb_fd = -1;
  sbsize_fd = -1;
}

int NDL_Init(uint32_t flags) {
  /* Initialization for screen width and height */
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
      if (strcmp(key, "WIDTH") == 0)          screen_w = atoi(value);
      else if (strcmp(key, "HEIGHT") == 0)    screen_h = atoi(value);
    }
  }

  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  return 0;
}

void NDL_Quit() {
}
