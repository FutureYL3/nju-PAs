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
  for (int i = 0; i < 6; ++ i)  CallbackHelper();
  uint32_t start = NDL_GetTicks();
  uint32_t now;
  while (NDL_GetTicks() - start < ms) {
    // busy waiting
  }
}

uint32_t SDL_GetTicks() {
  /* do not invoke CallbackHelper here, or it will trigger segmentation fault(reason unknown yet) */
  // for (int i = 0; i < 6; ++ i)  CallbackHelper();
  return NDL_GetTicks();
}