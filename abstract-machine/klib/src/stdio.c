#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>
#include <limits.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int len = 0;
	while (*fmt) {
		if (*fmt == '%') {
			++fmt;
      if (!*fmt) {
        panic("%% at the end of the format string\n");
      }
      char pad = ' ';
      int align = 1; // 0 for left and 1 for right
      int width = 0;
      switch (*fmt) {
        case '0':
          pad = '0'; ++fmt;
          break;
        case '-':
          align = 0;
          ++fmt;
          switch (*fmt) {
            case '0':
              pad = '0'; ++fmt;
              break;
          }
          break;
      }
      while (*fmt >= '0' && *fmt <= '9') {
        width = width * 10 + (*fmt - '0');
        fmt++;
      }
			switch (*fmt) {
				case 's':
					char *str = va_arg(ap, char *);
					while (str != NULL && *str) {
						putch(*str);
						++len;
            ++str;
					}
					break;
				case 'd':
					char buffer[20] = {0};
					int num = va_arg(ap, int), i = 0, is_neg = 0, is_min = 0;

					if (num == 0) {
            int pad_num = 0;
            if (width > 1)  pad_num = width - 1;

            if (align == 1) {
              for (int p = 0; p < pad_num; ++ p) {
                putch(pad);
                ++len;
              }
            }
            putch('0');
            ++len;
            if (align == 0) {
              for (int p = 0; p < pad_num; ++ p) {
                putch(pad);
                ++len;
              }
            }
						break;
					}

					if (num < 0) {
						is_neg = 1;
            if (num == INT_MIN) {
              is_min = 1;
              num = INT_MAX;
            }
            else  num = -num;
					}

					while (num) {
						buffer[i++] = num % 10 + '0';
						num /= 10;
					}
					if (is_neg) {
						buffer[i++] = '-';
					}
					int start = 0, end = i - 1;
					while (start < end) {
						char temp = buffer[start];
					 	buffer[start] = buffer[end];
						buffer[end] = temp;
						++start; --end;
					}
          if (is_min)  buffer[i - 1] += 1; // fix for INT_MIN
					len += i;

          int pad_num = 0;
          if (width > i)  pad_num = width - i; 
          
          if (align == 1) {
            for (int p = 0; p < pad_num; p++) {
              putch(pad);
              len++;
            }
          }

          for (int j = 0; j < i; j++)  putch(buffer[j]);

          if (align == 0) {
            for (int p = 0; p < pad_num; p++) {
              putch(pad);
              len++;
            }
          }
					break;
				default:
					halt(1);
			}
		}
		else {
			putch(*fmt);
			++len;
		}
		++fmt;
	}
	va_end(ap);
	return len;

}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int len = 0;
	while (*fmt) {
		if (*fmt == '%') {
			++fmt;
      if (!*fmt) {
        panic("%% at the end of the format string\n");
      }
			switch (*fmt) {
				case 's':
					char *str = va_arg(ap, char *);
					while (*str) {
						out[len++] = *str;
						++str;
					}
					break;
				case 'd':
					int num = va_arg(ap, int), i = 0, is_neg = 0, is_min = 0;

					if (num == 0) {
						out[len++] = '0';
						break;
					}

					if (num < 0) {
						is_neg = 1;
            if (num == INT_MIN) {
              is_min = 1;
              num = INT_MAX;
            }
            else  num = -num;
					}

					while (num) {
						out[len + i++] = num % 10 + '0';
						num /= 10;
					}
					if (is_neg) {
						out[len + i++] = '-';
					}
					int start = 0, end = i - 1;
					while (start < end) {
						char temp = out[len + start];
					 	out[len + start] = out[len + end];
						out[len + end] = temp;
						++start; --end;
					}
          if (is_min)  out[len + i - 1] += 1; // fix for INT_MIN
					len += i;
					break;
				default:
					halt(1);
			}
		}
		else {
			out[len++] = *fmt;
		}
		++fmt;
	}
	out[len] = '\0';
	va_end(ap);
	return len;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
