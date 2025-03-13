#include <am.h>
#include <nemu.h>

uint32_t vaddr_read(uint32_t addr, int len);

void __am_timer_init() {
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
	uint32_t upper = vaddr_read(RTC_ADDR + 4, 4);
	uint32_t lower = vaddr_read(RTC_ADDR, 4);
	uptime->us = 0;
	uptime->us += upper;
	uptime->us <<= 32;
	uptime->us += lower;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
