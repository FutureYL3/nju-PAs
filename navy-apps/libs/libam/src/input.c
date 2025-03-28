#include <am.h>
#include <NDL.h>
#include <string.h>

#define keyname(k) #k,

#define _KEYS(_) \
  _(ESCAPE) _(F1) _(F2) _(F3) _(F4) _(F5) _(F6) _(F7) _(F8) _(F9) _(F10) _(F11) _(F12) \
  _(GRAVE) _(1) _(2) _(3) _(4) _(5) _(6) _(7) _(8) _(9) _(0) _(MINUS) _(EQUALS) _(BACKSPACE) \
  _(TAB) _(Q) _(W) _(E) _(R) _(T) _(Y) _(U) _(I) _(O) _(P) _(LEFTBRACKET) _(RIGHTBRACKET) _(BACKSLASH) \
  _(CAPSLOCK) _(A) _(S) _(D) _(F) _(G) _(H) _(J) _(K) _(L) _(SEMICOLON) _(APOSTROPHE) _(RETURN) \
  _(LSHIFT) _(Z) _(X) _(C) _(V) _(B) _(N) _(M) _(COMMA) _(PERIOD) _(SLASH) _(RSHIFT) \
  _(LCTRL) _(APPLICATION) _(LALT) _(SPACE) _(RALT) _(RCTRL) \
  _(UP) _(DOWN) _(LEFT) _(RIGHT) _(INSERT) _(DELETE) _(HOME) _(END) _(PAGEUP) _(PAGEDOWN)

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

#define NR_KEYS (sizeof(keyname) / sizeof(keyname[0]))

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  char buf[64] = {0};
  int ret = NDL_PollEvent(buf, sizeof(buf));

  if (ret == 1) {
    buf[2] = '\0';
    char *key_op = buf;
    char *key_name = buf + 3;
    /* remove the tailing `\n` of key_name */
    for (char *p = key_name; ; ++ p) {
      if (*p == '\n') {
        *p = '\0';
        break;
      }
    }

    if (strcmp(key_op, "kd") == 0) {
      kbd->keydown = true;
    } 
    else if (strcmp(key_op, "ku") == 0) {
      kbd->keydown = false;
    }

    for (int i = 0; i < NR_KEYS; ++ i) {
      if (strcmp(key_name, keyname[i]) == 0)  {
        kbd->keycode = i;
        break;
      }
    }
    return;
  }

  kbd->keydown = false;
  kbd->keycode = 0;
}