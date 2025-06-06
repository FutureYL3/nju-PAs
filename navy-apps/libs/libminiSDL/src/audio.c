#include <NDL.h>
#include <SDL.h>
#include <stdlib.h>
#include "/home/yl/ics2022/navy-apps/libs/libfixedptc/include/fixedptc.h"

#define not_implemented 0

/* move the function out of condition compilation block if implemented */
#if not_implemented


void SDL_MixAudio(uint8_t *dst, uint8_t *src, uint32_t len, int volume) {
}

SDL_AudioSpec *SDL_LoadWAV(const char *file, SDL_AudioSpec *spec, uint8_t **audio_buf, uint32_t *audio_len) {
  return NULL;
}

void SDL_FreeWAV(uint8_t *audio_buf) {
}

#endif

uint8_t *buf;
void (*user_callback)(void *userdata, uint8_t *stream, int len);

int is_paused = 1; // 0 for not paused and 1 for paused
int is_locked = 0; // 0 for not locked and 1 for locked
/* audio config */
int freq;
int channels;
int samples;
size_t BUFFER_SIZE;

int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) {
  freq = desired->freq;
  channels = desired->channels;
  samples = desired->samples;
  NDL_OpenAudio(freq, channels, samples);
  is_paused = 1; // pause the audio
  user_callback = desired->callback;
  int byte_per_sample;
  switch (desired->format) {
    case AUDIO_U8: {
      byte_per_sample = 1;
      break;
    }
    case AUDIO_S16: {
      byte_per_sample = 2;
      break;
    }
  }
  /* allocate memory for buffer according to audio config */
  buf = (uint8_t *) malloc(channels * samples * byte_per_sample);
  BUFFER_SIZE = channels * samples * byte_per_sample;
  // printf("size of buf = %d\n", channels * samples * byte_per_sample);

  return 0;
}

void SDL_CloseAudio() {
  NDL_CloseAudio();
  is_paused = 1;
}

/* pause_on: 0 for not paused, 1 for paused */
void SDL_PauseAudio(int pause_on) {
  is_paused = pause_on;
}

fixedpt interval_ms = -1;
fixedpt last_ms = -1;
void CallbackHelper() {
  fixedpt fsamples = fixedpt_fromint(samples);
  fixedpt ffreq = fixedpt_fromint(freq);
  // printf("2\n");
  if (interval_ms == -1)  interval_ms = fixedpt_mul(fixedpt_div(fsamples, ffreq), fixedpt_fromint(1000));
  if (last_ms == -1)      last_ms = fixedpt_fromint(NDL_GetTicks());
  /* when paused or locked, we don't retrive any data by callback */
  if (is_paused || is_locked)  return;
  // printf("3\n");
  fixedpt current_ms = fixedpt_fromint(NDL_GetTicks());
  if (current_ms - last_ms < interval_ms)  return;
  // printf("4\n");
  int avai_space = NDL_QueryAudio();
  // printf("5\n");
  // printf("Callback helper\n");
  if (avai_space > BUFFER_SIZE) {
    // printf("6\n");
    user_callback(NULL, buf, BUFFER_SIZE);
    // printf("7\n");
    NDL_PlayAudio(buf, BUFFER_SIZE);
    // printf("8\n");
    // printf("NDL_PlayAudio\n");
  }
  
  last_ms = current_ms;
}

void SDL_LockAudio() {
  is_locked = 1;
}

void SDL_UnlockAudio() {
  is_locked = 0;
}
