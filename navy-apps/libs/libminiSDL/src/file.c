#include <sdl-file.h>


#define not_implemented 0

/* move the function out of condition compilation block if implemented */
#if not_implemented
SDL_RWops* SDL_RWFromFile(const char *filename, const char *mode) {
  return NULL;
}

SDL_RWops* SDL_RWFromMem(void *mem, int size) {
  return NULL;
}
#endif