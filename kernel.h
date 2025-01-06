#pragma once
#include "common.h"

struct sbiret {
  long error;
  long value;
};

struct trap_frame {
    uint32_t ra;
    uint32_t gp;
    uint32_t tp;
    uint32_t t0;
    uint32_t t1;
    uint32_t t2;
    uint32_t t3;
    uint32_t t4;
    uint32_t t5;
    uint32_t t6;
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t a7;
    uint32_t s0;
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
    uint32_t sp;
} __attribute__((packed));

#define PROCS_MAX 8
#define PROC_UNUSED 0
#define PROC_RUNNABLE 1

struct process {
  int pid;
  int state;
  vaddr_t sp;
  uint32_t *page_table;
  uint8_t stack[8192];
};

// virtual table
#define SATP_SV32 (1u << 31)
#define PAGE_V (1 << 0) // valid bit
#define PAGE_R (1 << 1) // read bit
#define PAGE_W (1 << 2) // write bit
#define PAGE_X (1 << 3) // execute bit
#define PAGE_U (1 << 4) // user bit

#define READ_CSR(reg)                                                      \
  ({                                                                       \
    unsigned long __tmp;                                                   \
    __asm__ __volatile__("csrr %0, " #reg : "=r"(__tmp));                  \
    __tmp;                                                                 \
  })

#define WRITE_CSR(reg, value)                                              \
  do {                                                                     \
    uint32_t __tmp = (value);                                              \
    __asm__ __volatile__("csrw " #reg ", %0" ::"r"(__tmp));                \
  } while (0)

#define PANIC(fmt, ...)                                                        \
    do {                                                                       \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
        while (1) {}                                                           \
    } while (0) // only a single loop. this is for executing multiple-line macros
