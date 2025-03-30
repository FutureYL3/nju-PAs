#include <common.h>
#include <fs.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

int screen_w = 0, screen_h = 0;
int sb_size = 0;

size_t serial_write(const void *buf, size_t offset, size_t len) {
  char *p = (char *) buf;
  for (int i = 0; i < len; ++ i) {
    putch(p[i]);
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  if (len == 0)  return -1;

  AM_INPUT_KEYBRD_T kbd = io_read(AM_INPUT_KEYBRD);
  bool keydown = kbd.keydown;
  int keycode = kbd.keycode;
  /* no valid key event, return 0 and do nothing on `buf` */
  if (keycode == 0)  return 0;

  memset(buf, 0, len);
  const char *key_name = keyname[keycode];
  char buffer[64] = {0};
  char *key_op = keydown ? "kd " : "ku ";
  strcpy(buffer, key_op); strcat(buffer, key_name);
  char *p = (char *) buf;
  int written = 0;
  /* write at most len - 1 characters due to making room for \n */
  for (int i = 0; i < len - 1 && buffer[i] != '\0'; ++ i) {
    p[i] = buffer[i];
    written++;
  }
  p[written++] = '\n';
  return written;
}

/* the return value spec of this function is the same as snprintf */
size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  if (len == 0)  return -1;

  AM_GPU_CONFIG_T cfg = io_read(AM_GPU_CONFIG);
  int h = cfg.height;
  int w = cfg.width;
  char *p = (char *) buf;
  int ret = snprintf(p, len, "WIDTH:%d\nHEIGHT:%d", w, h);
  return ret;
}

/* 
  This function can handle cross-line write, if upper interface doesn't 
  need this feature, it can be removed to improve performance.
*/
/*
  2025.3.30 update: sync at last write and remove the feature
*/
size_t fb_write(const void *buf, size_t offset, size_t len) {
  int x = (offset / 4) % screen_w, y = (offset / 4) / screen_w;

  uint32_t *p = (uint32_t *) buf;

  int w = len >> 16;
  int h = len & 0xffff;
  int written_bytes = w * h * 4;

  /* updated: remove this feature */
  // size_t fb_size = screen_w * screen_h * 4;
  // if (offset + written_bytes > fb_size) {
  //   Log("offset plus len exceeds frame buffer size in fb_write, just write to the remain space");
  //   len = fb_size - offset;
  // }

  AM_GPU_FBDRAW_T ctl = {
    .x = x, 
    .y = y,
    .pixels = (void *) p,
    .h = h,
    .w = w,
    .sync = true // sync when finish writing
  };

  /* we don't use io_write for readability */
  ioe_write(AM_GPU_FBDRAW, &ctl);

  return written_bytes;

  // while (x + pixels_to_write > screen_w) {
  //   size_t write_len = screen_w - x;
  //   AM_GPU_FBDRAW_T ctl = {
  //     .x = x, 
  //     .y = y, 
  //     .pixels = (void *) p,
  //     .h = 1,
  //     .w = write_len,
  //     .sync = false // sync when finish writing
  //   };
  //   /* we don't use io_write for readability */
  //   ioe_write(AM_GPU_FBDRAW, &ctl);
  //   /* update for potential next line write */
  //   pixels_to_write -= write_len;
  //   x = 0; y++;
  //   p += write_len;
  // }
  // /* when reach here, left bytes will not take up a whole screen width */
  // AM_GPU_FBDRAW_T ctl = {
  //   .x = x, 
  //   .y = y, 
  //   .pixels = (void *) p,
  //   .h = 1,
  //   .w = pixels_to_write,
  //   .sync = true
  // };
  // ioe_write(AM_GPU_FBDRAW, &ctl);

  // return original_len;
}

/* the arguments are not used */
size_t sbctl_read(void *buf, size_t offset, size_t len) {
  AM_AUDIO_STATUS_T stat = io_read(AM_AUDIO_STATUS);
  return sb_size - stat.count;
}

size_t sb_write(const void *buf, size_t offset, size_t len) {
  int avai_space = sbctl_read(NULL, 0, 0);
  while (avai_space < len) {
    /* wait until has enough space */
    avai_space = sbctl_read(NULL, 0, 0);
  }

  Area sb_buf = {.start = (void *) buf, .end = (void *) ((char *) buf + len)};
  io_write(AM_AUDIO_PLAY, sb_buf);
  return len;
}


size_t sbctl_write(const void *buf, size_t offset, size_t len) {
  
  assert(len == 12);  // for 3 int(4 bytes)
  int *p = (int *) buf;
  // printf("Get audio config, len = %d, freq = %d, channels = %d and samples = %d\n", len, p[0], p[1], p[2]);
  /* the order is: freq, channels, samples */
  io_write(AM_AUDIO_CTRL, p[0], p[1], p[2]);
  return len;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
  // /* get screen height and width */
  // char buf[64] = {0}; // 64 should be enough
  // fs_read(FD_DISPINFO, buf, sizeof(buf));
  // fs_close(FD_DISPINFO);
  // /* 5 lines should be enough */
  // char *lines[5];
  // int num_lines = 0;
  // char *line = strtok(buf, "\n");
  // while (line != NULL && num_lines < 5) {
  //   lines[num_lines++] = line;
  //   line = strtok(NULL, "\n");
  // }
  // for (int i = 0; i < num_lines; i++) {
  //   char *key = strtok(lines[i], " :");
  //   char *value = strtok(NULL, " :");
  //   if (key && value) {
  //     if (strcmp(key, "WIDTH") == 0)          screen_w = atoi(value);
  //     else if (strcmp(key, "HEIGHT") == 0)    screen_h = atoi(value);
  //   }
  // }
  AM_GPU_CONFIG_T gpu_cfg = io_read(AM_GPU_CONFIG);
  screen_w = gpu_cfg.width;
  screen_h = gpu_cfg.height;


  /* get sound buffer size */
  AM_AUDIO_CONFIG_T audio_cfg = io_read(AM_AUDIO_CONFIG);
  sb_size = audio_cfg.bufsize;
}
