#define SDL_malloc  malloc
#define SDL_free    free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"
#include <stdio.h>

SDL_Surface* IMG_Load_RW(SDL_RWops *src, int freesrc) {
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}

SDL_Surface* IMG_Load(const char *filename) {
  if (!filename)  return NULL;

  FILE *fp = fopen(filename, "rb");
  if (!fp)  return NULL;

  /* get file size */
  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  unsigned char *buf = (unsigned char *)SDL_malloc(size);
  if (!buf) {
    fclose(fp);
    return NULL;
  }
  /* read file content to the buffer */
  if (fread(buf, 1, size, fp) != size) {
    SDL_free(buf);
    fclose(fp);
    return NULL;
  }

  fclose(fp);
  /* load image to sdl surface */
  SDL_Surface *s = STBIMG_LoadFromMemory(buf, size);
  if (!s) {
    SDL_free(buf);
    return NULL;
  }

  SDL_free(buf);
  return s;
}

int IMG_isPNG(SDL_RWops *src) {
  return 0;
}

SDL_Surface* IMG_LoadJPG_RW(SDL_RWops *src) {
  return IMG_Load_RW(src, 0);
}

char *IMG_GetError() {
  return "Navy does not support IMG_GetError()";
}
