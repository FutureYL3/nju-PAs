#include <common.h>

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

size_t serial_write(const void *buf, size_t offset, size_t len) {
  char *p = (char *) buf;
  for (int i = 0; i < len; ++ i) {
    putch(p[i]);
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  AM_INPUT_KEYBRD_T kbd = io_read(AM_INPUT_KEYBRD);
  bool keydown = kbd.keydown;
  int keycode = kbd.keycode;
  /* no valid key event, return 0 and do nothing on `buf` */
  if (keycode == 0)  return 0;

  const char *key_name = keyname[keycode];
  char *key_op = keydown ? "kd " : "ku ";
  char *key_event = strcat(key_op, key_name);
  char *p = (char *) buf;
  int written = 0;
  /* write at most len - 1 characters due to making room for \n */
  while (*key_event != '\0' && written < len - 1) {
    p[written++] = *key_event;
    ++key_event;
  }
  p[written++] = '\n';
  return written;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  return 0;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
