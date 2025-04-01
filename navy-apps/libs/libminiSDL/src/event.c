#include <NDL.h>
#include <SDL.h>
#include <string.h>
#include <sdl-event.h>

#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

#define NR_KEYS (sizeof(keyname) / sizeof(keyname[0]))

#define not_implemented 0

/* move the function out of condition compilation block if implemented */
#if not_implemented
int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}



int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  return NULL;
}
#endif

int SDL_PollEvent(SDL_Event *ev) {
  CallbackHelper();
  CallbackHelper();
  CallbackHelper();
  CallbackHelper();
  CallbackHelper();
  CallbackHelper();
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
      ev->type = SDL_KEYDOWN;
    } 
    else if (strcmp(key_op, "ku") == 0) {
      ev->type = SDL_KEYUP;
    }

    for (int i = 0; i < NR_KEYS; ++ i) {
      if (strcmp(key_name, keyname[i]) == 0)  {
        ev->key.keysym.sym = i;
        break;
      }
    }

    return 1;
  }

  return 0;
}

int SDL_WaitEvent(SDL_Event *event) {
  while (true) {
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
        event->type = SDL_KEYDOWN;
      } 
      else if (strcmp(key_op, "ku") == 0) {
        event->type = SDL_KEYUP;
      }

      for (int i = 0; i < NR_KEYS; ++ i) {
        if (strcmp(key_name, keyname[i]) == 0)  {
          event->key.keysym.sym = i;
          break;
        }
      }

      return 1;
    }
  }

  return 0;
}