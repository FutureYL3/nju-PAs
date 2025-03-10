#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
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
			switch (*fmt) {
				case 's':
					char *str = va_arg(ap, char *);
					while (*str) {
						out[len++] = *str;
						++str;
					}
					break;
				case 'd':
					int num = va_arg(ap, int), i = 0, is_neg = 0;

					if (num == 0) {
						out[len++] = '0';
						break;
					}

					if (num < 0) {
						is_neg = 1;
						num = -num;
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
