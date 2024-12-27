#include "kernel.h"
#include "common.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

extern char __bss[], __bss_end[], __stack_top[], __free_ram[], __free_ram_end[];

// switch from supervisor mode to machine mode
// ecall itself can be more of an abstract call used when an upper layer wants to call a lower layer
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid) {
  // only a0-a1 will be modified by the ecall
  register long a0 __asm__("a0") = arg0;
  register long a1 __asm__("a1") = arg1;
  register long a2 __asm__("a2") = arg2;
  register long a3 __asm__("a3") = arg3;
  register long a4 __asm__("a4") = arg4;
  register long a5 __asm__("a5") = arg5;
  register long a6 __asm__("a6") = fid; // SBI function ID
  register long a7 __asm__("a7") = eid; // SBI extension ID

  __asm__ __volatile__("ecall" : "=r"(a0), "=r"(a1), "=r"(a2), "=r"(a3), "=r"(a4), "=r"(a5) : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7) : "memory");
  return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch) {
  sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}

// Linear memory allocation with no free
paddr_t alloc_pages(uint32_t n) {
  static paddr_t next_paddr = (paddr_t) __free_ram;
  paddr_t paddr = next_paddr;
  next_paddr += n * PAGE_SIZE;

  if (next_paddr > (paddr_t) __free_ram_end) {
    PANIC("out of memory");
  }

  memset((void *) paddr, 0, n * PAGE_SIZE);
  return paddr;
}

void handle_trap(struct trap_frame *f) {
  uint32_t scause = READ_CSR(scause);
  uint32_t stval = READ_CSR(stval);
  uint32_t user_pc = READ_CSR(sepc);

  PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
}

// Exception handler
__attribute__((naked))
__attribute__((aligned(4)))
void kernel_entry(void) {
  __asm__ __volatile__ (
    // csrrw is basically a swap operation
    // load the stack address of the current process from sscratch, which is stored in yield()
    // at the same time, store the current stack pointer (= sp of when the trap occurred) to sscratch
    "csrrw sp, sscratch, sp\n"
    // allocate space for 31 registers
    "addi sp, sp, -4 * 31\n"
    // Save the current registers (= process state) to the stack
    "sw ra, 4 * 0(sp)\n"
    "sw gp, 4 * 1(sp)\n"
    "sw tp, 4 * 2(sp)\n"
    "sw t0, 4 * 3(sp)\n"
    "sw t1, 4 * 4(sp)\n"
    "sw t2, 4 * 5(sp)\n"
    "sw t3, 4 * 6(sp)\n"
    "sw t4, 4 * 7(sp)\n"
    "sw t5, 4 * 8(sp)\n"
    "sw t6, 4 * 9(sp)\n"
    "sw a0, 4 * 10(sp)\n"
    "sw a1, 4 * 11(sp)\n"
    "sw a2, 4 * 12(sp)\n"
    "sw a3, 4 * 13(sp)\n"
    "sw a4, 4 * 14(sp)\n"
    "sw a5, 4 * 15(sp)\n"
    "sw a6, 4 * 16(sp)\n"
    "sw a7, 4 * 17(sp)\n"
    "sw s0, 4 * 18(sp)\n"
    "sw s1, 4 * 19(sp)\n"
    "sw s2, 4 * 20(sp)\n"
    "sw s3, 4 * 21(sp)\n"
    "sw s4, 4 * 22(sp)\n"
    "sw s5, 4 * 23(sp)\n"
    "sw s6, 4 * 24(sp)\n"
    "sw s7, 4 * 25(sp)\n"
    "sw s8, 4 * 26(sp)\n"
    "sw s9, 4 * 27(sp)\n"
    "sw s10, 4 * 28(sp)\n"
    "sw s11, 4 * 29(sp)\n"

    "csrr a0, sscratch\n" // Load the sp of when the trap occurred
    "sw a0, 4 * 30(sp)\n" // Save the stack pointer value to the stack

    // reset the stack pointer to the top of the stack
    "addi a0, sp, 4 * 31\n"
    "csrw sscratch, a0\n"

    "mv a0, sp\n" // Pass the stack pointer to a0
    "call handle_trap\n"

    "lw ra, 4 * 0(sp)\n"
    "lw gp, 4 * 1(sp)\n"
    "lw tp, 4 * 2(sp)\n"
    "lw t0, 4 * 3(sp)\n"
    "lw t1, 4 * 4(sp)\n"
    "lw t2, 4 * 5(sp)\n"
    "lw t3, 4 * 6(sp)\n"
    "lw t4, 4 * 7(sp)\n"
    "lw t5, 4 * 8(sp)\n"
    "lw t6, 4 * 9(sp)\n"
    "lw a0, 4 * 10(sp)\n"
    "lw a1, 4 * 11(sp)\n"
    "lw a2, 4 * 12(sp)\n"
    "lw a3, 4 * 13(sp)\n"
    "lw a4, 4 * 14(sp)\n"
    "lw a5, 4 * 15(sp)\n"
    "lw a6, 4 * 16(sp)\n"
    "lw a7, 4 * 17(sp)\n"
    "lw s0, 4 * 18(sp)\n"
    "lw s1, 4 * 19(sp)\n"
    "lw s2, 4 * 20(sp)\n"
    "lw s3, 4 * 21(sp)\n"
    "lw s4, 4 * 22(sp)\n"
    "lw s5, 4 * 23(sp)\n"
    "lw s6, 4 * 24(sp)\n"
    "lw s7, 4 * 25(sp)\n"
    "lw s8, 4 * 26(sp)\n"
    "lw s9, 4 * 27(sp)\n"
    "lw s10, 4 * 28(sp)\n"
    "lw s11, 4 * 29(sp)\n"
    "lw sp,  4 * 30(sp)\n"
    "sret\n"
  );
}

// context switch
struct process procs[PROCS_MAX];

__attribute__((naked))
void switch_context(uint32_t *prev_sp, uint32_t *next_sp) {
  __asm__ __volatile__(
    "addi sp, sp, -13 * 4\n"
    "sw ra, 4 * 0(sp)\n"
    "sw s0, 4 * 1(sp)\n"
    "sw s1, 4 * 2(sp)\n"
    "sw s2, 4 * 3(sp)\n"
    "sw s3, 4 * 4(sp)\n"
    "sw s4, 4 * 5(sp)\n"
    "sw s5, 4 * 6(sp)\n"
    "sw s6, 4 * 7(sp)\n"
    "sw s7, 4 * 8(sp)\n"
    "sw s8, 4 * 9(sp)\n"
    "sw s9, 4 * 10(sp)\n"
    "sw s10, 4 * 11(sp)\n"
    "sw s11, 4 * 12(sp)\n"
    "sw sp, (a0)\n" // store current sp to memory pointed by a0
    "lw sp, (a1)\n" // load next sp from memory pointed by a1
    "lw ra, 4 * 0(sp)\n"
    "lw s0, 4 * 1(sp)\n"
    "lw s1, 4 * 2(sp)\n"
    "lw s2, 4 * 3(sp)\n"
    "lw s3, 4 * 4(sp)\n"
    "lw s4, 4 * 5(sp)\n"
    "lw s5, 4 * 6(sp)\n"
    "lw s6, 4 * 7(sp)\n"
    "lw s7, 4 * 8(sp)\n"
    "lw s8, 4 * 9(sp)\n"
    "lw s9, 4 * 10(sp)\n"
    "lw s10, 4 * 11(sp)\n"
    "lw s11, 4 * 12(sp)\n"
    "addi sp, sp, 13 * 4\n"
    "ret\n"
  );
}

struct process *create_process(uint32_t pc) {
  // find an unused process
  struct process *proc = NULL;
  int i;
  for (i = 0; i < PROCS_MAX; i++) {
    if (procs[i].state == PROC_UNUSED) {
      proc = &procs[i];
      break;
    }
  }
  if (!proc) {
    PANIC("no available process slot");
  }
  // initialize process
  uint32_t *sp = (uint32_t *) &proc->stack[sizeof(proc->stack)];
  *--sp = 0;                      // s11
  *--sp = 0;                      // s10
  *--sp = 0;                      // s9
  *--sp = 0;                      // s8
  *--sp = 0;                      // s7
  *--sp = 0;                      // s6
  *--sp = 0;                      // s5
  *--sp = 0;                      // s4
  *--sp = 0;                      // s3
  *--sp = 0;                      // s2
  *--sp = 0;                      // s1
  *--sp = 0;                      // s0
  *--sp = (uint32_t) pc;          // ra

  proc->pid = i + 1;
  proc->state = PROC_RUNNABLE;
  proc->sp = (uint32_t) sp;
  return proc;
}

struct process *current_proc;
struct process *idle_proc;

// process scheduler
void yield(void) {
  struct process *next = idle_proc;
  // find executable process
  for (int i = 0; i < PROCS_MAX; i++) {
    struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
    if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
      next = proc;
      break;
    }
  }
  if (next == current_proc) {
    return;
  }

  // store the current process's stack pointer to sscratch
  __asm__ __volatile__(
    "csrw sscratch, %[sscratch]\n"
    :
    // sp address will be the end of next stack's address as stack is growing downwards
    : [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)])
  );

  struct process *prev = current_proc;
  current_proc = next;
  switch_context(&prev->sp, &next->sp);
}

struct process *proc_a;
struct process *proc_b;

void proc_a_entry(void) {
    printf("starting process A\n");
    while (1) {
        putchar('A');
        yield();

        for (int i = 0; i < 30000000; i++)
            __asm__ __volatile__("nop");
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (1) {
        putchar('B');
        yield();

        for (int i = 0; i < 30000000; i++)
            __asm__ __volatile__("nop");
    }
}

void kernel_main(void) {
  memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);
  
  WRITE_CSR(stvec, (uint32_t) kernel_entry);

  idle_proc = create_process((uint32_t) NULL);
  idle_proc->pid = -1;
  current_proc = idle_proc;

  proc_a = create_process((uint32_t) proc_a_entry);
  proc_b = create_process((uint32_t) proc_b_entry);

  yield();

  PANIC("kernel_main returned");
}

__attribute__((section(".text.boot"))) // place this function in .text.boot section
__attribute__((naked)) // naked attribute: no prologue/epilogue generated by compiler
void boot(void) {
  // set stack pointer, and jump to kernel_main
  __asm__ __volatile__ ( // volatile attribute: prevent compiler from optimizing this code (like -O0)
    "mv sp, %[stack_top]\n"
    "j kernel_main\n"
    :
    : [stack_top] "r" (__stack_top)
  );
}
