#include <NDL.h>
#include <sdl-video.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>


#define not_implemented 0

/* move the function out of condition compilation block if implemented */
#if not_implemented

#endif

void SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, uint32_t color) {
  CallbackHelper();
  /* if dstrect is NULL, update the whole canvas */
  if (dstrect == NULL) {
    if (dst->format->BitsPerPixel == 32) {
      uint32_t *p = (uint32_t *) dst->pixels;
      int size = dst->w * dst->h;
      for (int i = 0; i < size; ++ i)  p[i] = color;
    }
    else if (dst->format->BitsPerPixel == 8) {
      uint8_t *p = (uint8_t *)dst->pixels;
      int size = dst->w * dst->h;
      
      uint8_t r = (color >> 16) & 0xff;
      uint8_t g = (color >> 8) & 0xff;
      uint8_t b = color & 0xff;
      uint8_t idx = 0;
      
      for (int i = 0; i < dst->format->palette->ncolors; i++) {
        SDL_Color *pal_color = &dst->format->palette->colors[i];
        if (pal_color->r == r && pal_color->g == g && pal_color->b == b) {
          idx = i;
          break;
        }
      }
      
      for (int i = 0; i < size; ++i)  p[i] = idx;
    }
    SDL_UpdateRect(dst, 0, 0, dst->w, dst->h);
    return;
  }

  /* else, update the rect area */
  if (dst->format->BitsPerPixel == 32) {
    uint32_t *p = (uint32_t *) dst->pixels + dstrect->y * (dst->pitch / 4) + dstrect->x;
    for (int i = 0; i < dstrect->h; ++ i) {
      for (int j = 0; j < dstrect->w; ++ j) {
        p[j] = color;
      }
      p += dst->pitch / 4;
    }
  }
  else if (dst->format->BitsPerPixel == 8) {
    uint8_t *p = (uint8_t *)dst->pixels + dstrect->y * dst->pitch + dstrect->x;
   
    uint8_t r = (color >> 16) & 0xff;
    uint8_t g = (color >> 8) & 0xff;
    uint8_t b = color & 0xff;
    uint8_t idx = 0;
    
    for (int i = 0; i < dst->format->palette->ncolors; i++) {
      SDL_Color *pal_color = &dst->format->palette->colors[i];
      if (pal_color->r == r && pal_color->g == g && pal_color->b == b) {
        idx = i;
        break;
      }
    }
    
    for (int i = 0; i < dstrect->h; ++i) {
      for (int j = 0; j < dstrect->w; ++j) {
        p[j] = idx;
      }
      p += dst->pitch;
    }
  }
  SDL_UpdateRect(dst, dstrect->x, dstrect->y, dstrect->w, dstrect->h);
}

void SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  CallbackHelper();
  assert(dst && src);
  assert(dst->format->BitsPerPixel == src->format->BitsPerPixel);

  // if srcrect is NULL，copy the entire src surface
  SDL_Rect src_rect;
  if (srcrect == NULL) {
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.w = src->w;
    src_rect.h = src->h;
    srcrect = &src_rect;
  }

  // if dstrect is NULL，start copy to (0,0)
  SDL_Rect dst_rect;
  if (dstrect == NULL) {
    dst_rect.x = 0;
    dst_rect.y = 0;
    dstrect = &dst_rect;
  }

  // check for border
  if (srcrect->x < 0 || srcrect->y < 0 || 
      srcrect->x + srcrect->w > src->w || 
      srcrect->y + srcrect->h > src->h ||
      dstrect->x < 0 || dstrect->y < 0 ||
      dstrect->x + srcrect->w > dst->w || 
      dstrect->y + srcrect->h > dst->h) {
    return;
  }

  // copy according to pixel representation
  if (src->format->BitsPerPixel == 32) {
    uint32_t *src_pixels = (uint32_t *)src->pixels + srcrect->y * (src->pitch / 4) + srcrect->x;
    uint32_t *dst_pixels = (uint32_t *)dst->pixels + dstrect->y * (dst->pitch / 4) + dstrect->x;
    
    for (int i = 0; i < srcrect->h; i++) {
      memcpy(dst_pixels, src_pixels, srcrect->w * sizeof(uint32_t));
      src_pixels += src->pitch / 4;
      dst_pixels += dst->pitch / 4;
    }
  }
  else if (src->format->BitsPerPixel == 8) {
    uint8_t *src_pixels = (uint8_t *)src->pixels + srcrect->y * src->pitch + srcrect->x;
    uint8_t *dst_pixels = (uint8_t *)dst->pixels + dstrect->y * dst->pitch + dstrect->x;
    
    for (int i = 0; i < srcrect->h; i++) {
      memcpy(dst_pixels, src_pixels, srcrect->w);
      src_pixels += src->pitch;
      dst_pixels += dst->pitch;
    }
  }

  // update
  SDL_UpdateRect(dst, dstrect->x, dstrect->y, srcrect->w, srcrect->h);
}

/* 
  Makes sure the given area is updated on the given screen. 
  The rectangle must be confined within the screen boundaries (no clipping is done). 
*/
void SDL_UpdateRect(SDL_Surface *s, int x, int y, int w, int h) {
  for (int i = 0; i < 40; ++ i)  CallbackHelper();
  // printf("self implemented SDL_UpdateRect called\n");
  if (s == NULL) return;
  if (w < 0 || h < 0) return;
  /* If 'x', 'y', 'w' and 'h' are all 0, SDL_UpdateRect will update the entire screen. */
  if (w == 0 && h == 0 && x == 0 && y == 0) {
    w = s->w;
    h = s->h;
  }
  /* check for pixel representation mode */
  if (s->format->BitsPerPixel == 32) {
    /* 32 bits color mode */
    // Allocate a temporary buffer for the rectangle's pixels
    uint32_t *rect_pixels = malloc(w * h * sizeof(uint32_t));
    if (rect_pixels == NULL) {
      fprintf(stderr, "Failed to allocate memory for rectangle pixels\n");
      return;
    }
    // Copy pixels from surface to the temporary buffer
    uint32_t *src = (uint32_t *)s->pixels + y * (s->pitch / 4) + x;
    uint32_t *dst = rect_pixels;
    for (int i = 0; i < h; i++) {
      memcpy(dst, src, w * sizeof(uint32_t));
      src += s->pitch / 4;  // Move to next line in source
      dst += w;             // Move to next line in destination
    }
    // Draw the rectangle using the temporary buffer
    NDL_DrawRect(rect_pixels, x, y, w, h);
    free(rect_pixels);
  } 
  else if (s->format->BitsPerPixel == 8) {
    /* palette mode */
    uint32_t *pixels = malloc(w * h * sizeof(uint32_t));
    if (pixels == NULL) {
      fprintf(stderr, "Failed to allocate memory for pixel buffer at palette mode\n");
      return;
    }

    uint8_t *src = (uint8_t *)s->pixels + y * s->pitch + x;
    SDL_Color *colors = s->format->palette->colors;
    
    for (int i = 0; i < h; i++) {
      for (int j = 0; j < w; j++) {
        uint8_t idx = src[i * s->pitch + j];
        SDL_Color color = colors[idx];
        pixels[i * w + j] = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
      }
    }
    
    NDL_DrawRect(pixels, x, y, w, h);
    free(pixels);
  }
}

// APIs below are already implemented.

static inline int maskToShift(uint32_t mask) {
  switch (mask) {
    case 0x000000ff: return 0;
    case 0x0000ff00: return 8;
    case 0x00ff0000: return 16;
    case 0xff000000: return 24;
    case 0x00000000: return 24; // hack
    default: assert(0);
  }
}

SDL_Surface* SDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth,
    uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
  assert(depth == 8 || depth == 32);
  SDL_Surface *s = malloc(sizeof(SDL_Surface));
  assert(s);
  s->flags = flags;
  s->format = malloc(sizeof(SDL_PixelFormat));
  assert(s->format);
  if (depth == 8) {
    s->format->palette = malloc(sizeof(SDL_Palette));
    assert(s->format->palette);
    s->format->palette->colors = malloc(sizeof(SDL_Color) * 256);
    assert(s->format->palette->colors);
    memset(s->format->palette->colors, 0, sizeof(SDL_Color) * 256);
    s->format->palette->ncolors = 256;
  } else {
    s->format->palette = NULL;
    s->format->Rmask = Rmask; s->format->Rshift = maskToShift(Rmask); s->format->Rloss = 0;
    s->format->Gmask = Gmask; s->format->Gshift = maskToShift(Gmask); s->format->Gloss = 0;
    s->format->Bmask = Bmask; s->format->Bshift = maskToShift(Bmask); s->format->Bloss = 0;
    s->format->Amask = Amask; s->format->Ashift = maskToShift(Amask); s->format->Aloss = 0;
  }

  s->format->BitsPerPixel = depth;
  s->format->BytesPerPixel = depth / 8;

  s->w = width;
  s->h = height;
  s->pitch = width * depth / 8;
  assert(s->pitch == width * s->format->BytesPerPixel);

  if (!(flags & SDL_PREALLOC)) {
    s->pixels = malloc(s->pitch * height);
    assert(s->pixels);
  }

  return s;
}

SDL_Surface* SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth,
    int pitch, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
  SDL_Surface *s = SDL_CreateRGBSurface(SDL_PREALLOC, width, height, depth,
      Rmask, Gmask, Bmask, Amask);
  assert(pitch == s->pitch);
  s->pixels = pixels;
  return s;
}

void SDL_FreeSurface(SDL_Surface *s) {
  if (s != NULL) {
    if (s->format != NULL) {
      if (s->format->palette != NULL) {
        if (s->format->palette->colors != NULL) free(s->format->palette->colors);
        free(s->format->palette);
      }
      free(s->format);
    }
    if (s->pixels != NULL && !(s->flags & SDL_PREALLOC)) free(s->pixels);
    free(s);
  }
}

SDL_Surface* SDL_SetVideoMode(int width, int height, int bpp, uint32_t flags) {
  if (flags & SDL_HWSURFACE) NDL_OpenCanvas(&width, &height);
  return SDL_CreateRGBSurface(flags, width, height, bpp,
      DEFAULT_RMASK, DEFAULT_GMASK, DEFAULT_BMASK, DEFAULT_AMASK);
}

void SDL_SoftStretch(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  assert(src && dst);
  assert(dst->format->BitsPerPixel == src->format->BitsPerPixel);
  assert(dst->format->BitsPerPixel == 8);

  int x = (srcrect == NULL ? 0 : srcrect->x);
  int y = (srcrect == NULL ? 0 : srcrect->y);
  int w = (srcrect == NULL ? src->w : srcrect->w);
  int h = (srcrect == NULL ? src->h : srcrect->h);

  assert(dstrect);
  if(w == dstrect->w && h == dstrect->h) {
    /* The source rectangle and the destination rectangle
     * are of the same size. If that is the case, there
     * is no need to stretch, just copy. */
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SDL_BlitSurface(src, &rect, dst, dstrect);
  }
  else {
    assert(0);
  }
}

void SDL_SetPalette(SDL_Surface *s, int flags, SDL_Color *colors, int firstcolor, int ncolors) {
  assert(s);
  assert(s->format);
  assert(s->format->palette);
  assert(firstcolor == 0);

  s->format->palette->ncolors = ncolors;
  memcpy(s->format->palette->colors, colors, sizeof(SDL_Color) * ncolors);

  if(s->flags & SDL_HWSURFACE) {
    assert(ncolors == 256);
    for (int i = 0; i < ncolors; i ++) {
      uint8_t r = colors[i].r;
      uint8_t g = colors[i].g;
      uint8_t b = colors[i].b;
    }
    SDL_UpdateRect(s, 0, 0, 0, 0);
  }
}

static void ConvertPixelsARGB_ABGR(void *dst, void *src, int len) {
  int i;
  uint8_t (*pdst)[4] = dst;
  uint8_t (*psrc)[4] = src;
  union {
    uint8_t val8[4];
    uint32_t val32;
  } tmp;
  int first = len & ~0xf;
  for (i = 0; i < first; i += 16) {
#define macro(i) \
    tmp.val32 = *((uint32_t *)psrc[i]); \
    *((uint32_t *)pdst[i]) = tmp.val32; \
    pdst[i][0] = tmp.val8[2]; \
    pdst[i][2] = tmp.val8[0];

    macro(i + 0); macro(i + 1); macro(i + 2); macro(i + 3);
    macro(i + 4); macro(i + 5); macro(i + 6); macro(i + 7);
    macro(i + 8); macro(i + 9); macro(i +10); macro(i +11);
    macro(i +12); macro(i +13); macro(i +14); macro(i +15);
  }

  for (; i < len; i ++) {
    macro(i);
  }
}

SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, SDL_PixelFormat *fmt, uint32_t flags) {
  assert(src->format->BitsPerPixel == 32);
  assert(src->w * src->format->BytesPerPixel == src->pitch);
  assert(src->format->BitsPerPixel == fmt->BitsPerPixel);

  SDL_Surface* ret = SDL_CreateRGBSurface(flags, src->w, src->h, fmt->BitsPerPixel,
    fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);

  assert(fmt->Gmask == src->format->Gmask);
  assert(fmt->Amask == 0 || src->format->Amask == 0 || (fmt->Amask == src->format->Amask));
  ConvertPixelsARGB_ABGR(ret->pixels, src->pixels, src->w * src->h);

  return ret;
}

uint32_t SDL_MapRGBA(SDL_PixelFormat *fmt, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  assert(fmt->BytesPerPixel == 4);
  uint32_t p = (r << fmt->Rshift) | (g << fmt->Gshift) | (b << fmt->Bshift);
  if (fmt->Amask) p |= (a << fmt->Ashift);
  return p;
}

int SDL_LockSurface(SDL_Surface *s) {
  return 0;
}

void SDL_UnlockSurface(SDL_Surface *s) {
}
