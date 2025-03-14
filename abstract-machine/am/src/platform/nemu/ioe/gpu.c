#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)
#define N 32

void __am_gpu_init() {
	int i;
	uint32_t vga_size = inl(VGACTL_ADDR);
	int w = vga_size >> 16;
	int h = vga_size & 0xffff;
	uint32_t *fb = (uint32_t *)(uintptr_t) FB_ADDR;
	for (i = 0; i < w * h; ++ i)  fb[i] = i;
	outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint32_t vga_size = inl(VGACTL_ADDR);
	int w = (vga_size >> 16);
	int h = (vga_size & 0xffff);
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = w, .height = h,
    .vmemsz = w * h * N
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
	uint32_t vga_size = inl(VGACTL_ADDR);
	int w = (vga_size >> 16);
	uint32_t *pixels_ptr = (uint32_t *) ctl->pixels;
	uint32_t offset = (w * ctl->y + ctl->x) * 4;
	for (int i = 0; i < ctl->h; ++ i) {
		for (int j = 0; j < ctl->w; ++ j) {
			uint32_t pos = FB_ADDR + offset + 4 * j;
			outl(pos, *pixels_ptr);
			++pixels_ptr;
		}
		offset += w * 4;
	}

  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
