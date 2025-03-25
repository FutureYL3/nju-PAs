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

int SDL_PollEvent(SDL_Event *ev) {
  return 0;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  return NULL;
}
#endif

int SDL_WaitEvent(SDL_Event *event) {
  char buf[64] = {0}; // 64 should be enough
  int ret = NDL_PollEvent(buf, sizeof(buf));
  /* None event */
  if (ret == 0)  return 1;

  // char *key_op = strtok(buf, " ");
  // char *key_name = strtok(NULL, " ");
  buf[2] = '\0';
  char *key_op = buf;
  char *key_name = buf + 3;
  /* for type */
  if (strcmp(key_op, "kd") == 0)      event->type = SDL_KEYDOWN;
  else if (strcmp(key_op, "ku") == 0) event->type = SDL_KEYUP;
  /* for keysym */
  for (int i = 0; i < NR_KEYS; ++ i) {
    if (strcmp(key_name, keyname[i]) == 0)  {
      event->key.keysym.sym = i;
      break;
    }
  }
}