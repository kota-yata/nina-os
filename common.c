// common functions among s-mode and u-mode
#include "common.h"

void putchar(char ch);

void *memset(void *buf, char c, size_t n) {
  uint8_t *p = (uint8_t *) buf;
  while (n--) {
    *p++ = c;
  }
  return buf;
}

void *memcpy(void *dst, const void *src, size_t n) {
  uint8_t *d = (uint8_t *) dst;
  const uint8_t *s = (const uint8_t *) src;
  while(n--) {
    *d++ = *s++;
  }
  return dst;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;

  while (n--) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  return 0;
}

void *strcpy(char *dst, const char *src) {
  char *d = dst;
  while (*src) {
    *d++ = *src++;
  }
  *d = '\0';
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s2) {
    if (*s1 != *s2) {
      break;
    }
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2; // cast to unsigned char to align with POSIX standard
}

size_t strlen(const char *str) {
  const char *s = str;
  while (*s) {
      s++;
  }
  return s - str;
}


void printf(const char *fmt, ...) {
  va_list vargs;
  va_start(vargs, fmt);

  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt) {
        case '\0':
          putchar('%');
          goto end;
        case '%':
          putchar('%');
          break;
        case 's': {
          const char *s = va_arg(vargs, const char *);
          while (*s) {
            putchar(*s++);
          }
          break;
        }
        case 'd': {
          int value = va_arg(vargs, int);
          if (value < 0) {
            putchar('-');
            value = -value;
          }
          int divisor = 1;
          while (value / divisor >= 10) {
            divisor *= 10;
          }
          while (divisor > 0) {
            putchar('0' + value / divisor);
            value %= divisor;
            divisor /= 10;
          }

          break;
        }
        case 'x': {
          int value = va_arg(vargs, int);
          for (int i = 7; i >= 0; i--) {
            int nibble = (value >> (i * 4)) & 0xf;
            putchar("0123456789abcdef"[nibble]);
          }
        }
      }
    } else {
      putchar(*fmt);
    }
    fmt++;
  }
end:
  va_end(vargs);
}

uint16_t htons(uint16_t hostshort) {
  uint16_t test = 1;
  uint8_t *byte = (uint8_t *)&test;
  if (*byte == 1) {
      return (hostshort >> 8) | (hostshort << 8);
  }
  return hostshort;
}

uint32_t htonl(uint32_t hostlong) {
  uint16_t test = 1;
  uint8_t *byte = (uint8_t *)&test;
  if (*byte == 1) {
    return ((hostlong >> 24) & 0x000000FF) |
            ((hostlong >> 8) & 0x0000FF00) |
            ((hostlong << 8) & 0x00FF0000) |
            ((hostlong << 24) & 0xFF000000);
  }
  return hostlong;
}

uint16_t ntohs(uint16_t netshort) {
  uint16_t test = 1;
  uint8_t *byte = (uint8_t *)&test;
  if (*byte == 1) { // for little endian
    return (netshort >> 8) | (netshort << 8);
  }
  return netshort; // for big endian
}

// custom common functions
uint32_t address8to32(uint8_t *address) {
  return ((uint32_t)address[0] << 24) |
         ((uint32_t)address[1] << 16) |
         ((uint32_t)address[2] << 8) |
         ((uint32_t)address[3]);
}

