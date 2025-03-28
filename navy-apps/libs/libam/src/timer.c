#include <am.h>
#include <NDL.h>

__am_timer_init() {
  
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uint32_t ms = NDL_GetTicks();
  uptime->us = ms * 1000;
}