#pragma once

typedef int bool;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef uint32_t size_t;
typedef uint32_t paddr_t;
typedef uint32_t vaddr_t;

#define true 1
#define false 0
#define NULL ((void *) 0)
#define align_up(value, align) __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)
#define offsetof(type, member) __builtin_offsetof(type, member)
#define va_list __builtin_va_list
#define va_start  __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg
#define PAGE_SIZE 4096

// system call
#define SYS_PUTCHAR 1
#define SYS_GETCHAR 2
#define SYS_EXIT 3

void *memset(void *buf, char c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
void printf(const char *fmt, ...);
uint16_t htons(uint16_t hostshort);
uint32_t htonl(uint32_t hostlong);
uint16_t ntohs(uint16_t netshort);

static uint8_t MY_MAC_ADDRESS[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static uint8_t NULL_MAC_ADDRESS[6] = {0, 0, 0, 0, 0, 0};
static uint8_t MY_IP_ADDRESS[4] = {192, 168, 100, 104};
static uint32_t MY_IP_ADDRESS_32 = 0xC0A86468; // 192.168.100.104

uint32_t address8to32(uint8_t *address);
