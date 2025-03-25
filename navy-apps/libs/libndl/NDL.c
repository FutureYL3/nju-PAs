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

uint32_t NDL_GetTicks() {
  struct timeval tz = {};
  int ret = gettimeofday(&tz, NULL);
  if (ret < 0) {
    return -1;
  }
  // ms as unit
  return tz.tv_sec * 1000 + tz.tv_usec / 1000;
}

int NDL_PollEvent(char *buf, int len) {
  /* in sfs, we do not use flags and mode */
  int fd = open("/dev/events", 0, 0);
  int ret = read(fd, (void *) buf, len);
  if (ret == 0)  return 0;

  return 1;
}

void NDL_OpenCanvas(int *w, int *h) {
  if (*w > screen_w || *h > screen_h) {
    fprintf(stderr, "Canvas width/height exceeds screen width/height\n");
    return;
  }

  printf("The screen size: width %d, height %d\n", screen_w, screen_h);
  printf("The canvas size: width %d, height %d\n", *w, *h);
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

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
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
