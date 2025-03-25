#include <NDL.h>
#include <sdl-timer.h>
#include <stdio.h>


#define not_implemented 0

/* move the function out of condition compilation block if implemented */
#if not_implemented
SDL_TimerID SDL_AddTimer(uint32_t interval, SDL_NewTimerCallback callback, void *param) {
  return NULL;
}

int SDL_RemoveTimer(SDL_TimerID id) {
  return 1;
}

uint32_t SDL_GetTicks() {
  return 0;
}

void SDL_Delay(uint32_t ms) {
}
#endif