#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
	size_t len = 0;
	while (s[len] != '\0') 	++len;
	
	return len;
}

/* the dst and src shall not overlap, if violated, UB can happen */
/* if dst is not large enough, strcpy will override exceeding area's byte */
char *strcpy(char *dst, const char *src) {
	char *ret = dst;
	while (*src != '\0') {
		*dst = *src;	
		++dst; ++src;
	}
	*dst = *src;

	return ret;
}

/* if n == 0, the function will do nothing */
char *strncpy(char *dst, const char *src, size_t n) {
	size_t i;

	for (i = 0; i < n && src[i] != '\0'; ++ i)  dst[i] = src[i];
	for ( ; i < n; ++ i)  dst[i] = '\0';

	return dst;
}

char *strcat(char *dst, const char *src) {
	char *ret = dst;

	while (*dst != '\0')  ++dst;

	while (*src != '\0') {
		*dst = *src;
		++dst; ++src;
	}
	*dst = *src;

	return ret;
}

int strcmp(const char *s1, const char *s2) {
	while (*s1 != '\0' && *s2 != '\0') {
		int diff = *s1 - *s2;
		if (diff != 0)  return diff;
		++s1; ++s2;
	}
	if (*s1 == '\0' && *s2 == '\0')  return 0;
	else if (*s1 == '\0')  return -(*s2);
	else  return *s1;
}

/* if n == 0, the function will return 0 */
int strncmp(const char *s1, const char *s2, size_t n) {
	size_t i;
	for (i = 0; i < n && s1[i] != '\0' && s2[i] != '\0'; ++ i) {
		int diff = s1[i] - s2[i];
		if (diff != 0)  return diff;
	}
	if (i == n)  return 0;
	else {
		if (s1[i] == '\0' && s2[i] == '\0')  return 0;
		else if (s1[i] == '\0')  return -(s2[i]);
		else  return s1[i];
	}	
}

/* if n == 0, the function will do nothing */
void *memset(void *s, int c, size_t n) {
	size_t i;
	unsigned char *p = (unsigned char*) s;
	unsigned char val = (unsigned char) c;
	for (i = 0; i < n; ++ i) {
		*p = val;
		++p;
	}

	return s;
}

/* if n == 0, the function will do nothing */
void *memmove(void *dst, const void *src, size_t n) {
	unsigned char temp[n];
	memcpy((void*) temp, src, n);

	size_t i;
	unsigned char *p = (unsigned char*) dst;
	for (i = 0; i < n; ++ i) {
		*p = temp[i];
		++p;
	}

	return dst;
}

/* if n == 0, the function will do nothing */
void *memcpy(void *out, const void *in, size_t n) {
	unsigned char *dst = (unsigned char*) out;
	const unsigned char *src = (const unsigned char*) in;

	size_t i;
	for (i = 0; i < n; ++ i) {
		*dst = *src;
		++dst; ++src;
	}
	return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
	const unsigned char *p1 = (const unsigned char*) s1;
	const unsigned char *p2 = (const unsigned char*) s2;

	size_t i;
	for (i = 0; i < n; ++ i) {
		int diff = p1[i] - p2[i];
		if (diff != 0)  return diff;
	}

	return 0;
}

#endif
