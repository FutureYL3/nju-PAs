#include <NDL.h>
#include <sdl-timer.h>
#include <stdio.h>
#include <SDL.h>


#define not_implemented 0

/* move the function out of condition compilation block if implemented */
#if not_implemented
SDL_TimerID SDL_AddTimer(uint32_t interval, SDL_NewTimerCallback callback, void *param) {
  return NULL;
}

int SDL_RemoveTimer(SDL_TimerID id) {
  return 1;
}




#endif

void SDL_Delay(uint32_t ms) {
  CallbackHelper();
  CallbackHelper();
  CallbackHelper();
  CallbackHelper();
  CallbackHelper();
  CallbackHelper();
  uint32_t start = NDL_GetTicks();
  uint32_t now;
  while (NDL_GetTicks() - start < ms) {
    // busy waiting
  }
}

uint32_t SDL_GetTicks() {
  return NDL_GetTicks();
}