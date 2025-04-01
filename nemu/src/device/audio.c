/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#include <device/map.h>
#include <SDL2/SDL.h>

enum {
  reg_freq,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,
  nr_reg
};

static uint8_t *sbuf = NULL;
static uint32_t *audio_base = NULL;

static void audio_callback(void *userdata, Uint8 *stream, int len) {
	printf("audio_callback\n");
 	uint32_t count = audio_base[5];
	if (count < len) {
		int i;
		for (i = 0; i < count; ++ i) 	stream[i] = sbuf[i]; 
		for ( ; i < len; ++ i)  stream[i] = 0;
		audio_base[5] = 0;
	}
	else {
		for (int i = 0; i < len; ++ i)  stream[i] = sbuf[i];
    memmove(sbuf, sbuf + len, count - len);
		audio_base[5] = count - len;
	}
}

static void audio_io_handler(uint32_t offset, int len, bool is_write) {
	if (offset == 16 && is_write) {
		// init SDL audio card
		Assert(audio_base[0] != 0 && audio_base[1] != 0 && audio_base[2] != 0, "freq channles or samples register uninitialized\n");
		SDL_AudioSpec s = {};
		s.format = AUDIO_S16SYS;
		s.userdata = NULL;
		s.freq = audio_base[0];
		s.channels = audio_base[1];
		s.samples = audio_base[2];
		s.callback = audio_callback;
		Assert(SDL_InitSubSystem(SDL_INIT_AUDIO) == 0, "failed to init sdl audio subsystem\n");
		// printf("SDL_InitSubSystem\n");
		Assert(SDL_OpenAudio(&s, NULL) == 0, "failed to open audio spec\n");
		// printf("SDL_OpenAudio\n");
		SDL_PauseAudio(0); // start play
		// printf("SDL_PauseAudio\n");
		// printf("freq = %d, channels = %d, samples = %d\n", audio_base[0], audio_base[1], audio_base[2]);
		
	}

	if (offset == 0 || offset == 4 || offset == 8)  Assert(is_write, "freq, channels or samples register can not be read\n");
	if (offset == 12)  Assert(!is_write, "bufsize register can not be written\n");
}

void init_audio() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
	audio_base[3] = CONFIG_SB_SIZE;
	audio_base[4] = 0; // init reg is 0 for currently uninitialized
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
}
