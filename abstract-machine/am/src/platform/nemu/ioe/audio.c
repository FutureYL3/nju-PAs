#include <am.h>
#include <nemu.h>

#define AUDIO_FREQ_ADDR      (AUDIO_ADDR + 0x00)
#define AUDIO_CHANNELS_ADDR  (AUDIO_ADDR + 0x04)
#define AUDIO_SAMPLES_ADDR   (AUDIO_ADDR + 0x08)
#define AUDIO_SBUF_SIZE_ADDR (AUDIO_ADDR + 0x0c)
#define AUDIO_INIT_ADDR      (AUDIO_ADDR + 0x10)
#define AUDIO_COUNT_ADDR     (AUDIO_ADDR + 0x14)

void __am_audio_init() {
}

void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) {
  cfg->present = true;
	cfg->bufsize = inl(AUDIO_SBUF_SIZE_ADDR);
}

void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl) {
	outl(AUDIO_FREQ_ADDR, ctrl->freq);
	outl(AUDIO_CHANNELS_ADDR, ctrl->channels);
	outl(AUDIO_SAMPLES_ADDR, ctrl->samples);
	outl(AUDIO_INIT_ADDR, 1); // for init reg
}

void __am_audio_status(AM_AUDIO_STATUS_T *stat) {
  stat->count = inl(AUDIO_COUNT_ADDR);
}

void __am_audio_play(AM_AUDIO_PLAY_T *ctl) {
	// uint32_t sbuf_size = inl(AUDIO_SBUF_SIZE_ADDR);
	volatile uint32_t count = inl(AUDIO_COUNT_ADDR);
	uint32_t data_size = ctl->buf.end - ctl->buf.start;
	// while (sbuf_size - count < data_size) { /* wait for enough space */ }

	uint32_t base = AUDIO_SBUF_ADDR + count;
	uint8_t *dest = (uint8_t *) base;
	uint8_t *src  = (uint8_t *) ctl->buf.start;

	for (uint32_t i = 0; i < data_size; i++)  dest[i] = src[i];
	
	outl(AUDIO_COUNT_ADDR, count + data_size);
}
