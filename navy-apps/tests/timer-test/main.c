#include <stdio.h>
#include <sys/time.h>

int main() {
  long cur_sec = 0;
  long cur_usec = 0;
  struct timeval tv = {};
  int ret = gettimeofday(&tv, NULL);
  if (ret < 0) {
    printf("Failed to get sys time at init time\n");
    return -1;
  }
  cur_sec = tv.tv_sec;
  cur_usec = tv.tv_usec;
  int count = 1;
  // printf("start time: %ld.%ld\n", cur_sec, cur_usec);
  while (1) {
    ret = gettimeofday(&tv, NULL);
    if (ret < 0) {
      printf("Failed to get systime at %d times\n", count);
    }
    // printf("current time: %ld.%ld\n", tv.tv_sec, tv.tv_usec);
    if (tv.tv_sec == cur_sec) {
      /* tolerate 100000 us(0.1s) diff */
      if (tv.tv_usec - cur_usec > 450000 && tv.tv_usec - cur_usec < 550000) {
        printf("already pass %d 0.5s\n", count++);
        // ++count;
      }
      else continue;
    }
    else if (tv.tv_sec - cur_sec == 1) {
      long us = tv.tv_usec + 1000000;
      /* tolerate 100000 us(0.1s) diff */
      if (us - cur_usec > 450000 && us - cur_usec < 550000) {
        printf("already pass %d 0.5s\n", count++);
        // ++count;
      }
      else continue;
    }

    // if (count % 2 == 0)  printf("already pass %ds\n", count / 2);

    cur_sec = tv.tv_sec;
    cur_usec = tv.tv_usec;
  }

}