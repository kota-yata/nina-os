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

long getchar(void) {
  struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
  return ret.error;
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

// virtual table (user space)
// refer to https://vlsi.jp/UnderstandMMU.html when you get confused
// sv32 consists of 2-level page table entry
void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags){
  if (!is_aligned(vaddr, PAGE_SIZE)) {
    PANIC("unalined vaddr %x", vaddr);
  }
  if (!is_aligned(paddr, PAGE_SIZE)) {
    PANIC("unalined paddr %x", paddr);
  }

  uint32_t vpn1 = (vaddr >> 22) & 0x3ff; // crop the first 10 bits (32-22) and mask with 0x3ff (10 of 1s)

  if ((table1[vpn1] & PAGE_V) == 0) {
    uint32_t pt_paddr = alloc_pages(1);
    table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;
  }

  uint32_t vpn0 = (vaddr >> 12) & 0x3ff;

  uint32_t *table0 = (uint32_t *) ((table1[vpn1] >> 10) * PAGE_SIZE); // basically trying to get pt_paddr

  table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}

// virtio
uint32_t virtio_reg_read32(unsigned offset) {
  return *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset));
}
uint64_t virtio_reg_read64(unsigned offset) {
  return *((volatile uint64_t *) (VIRTIO_BLK_PADDR + offset));
}
void virtio_reg_write32(unsigned offset, uint32_t value) {
  *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset)) = value;
}

void virtio_reg_fetch_and_or32(unsigned offset, uint32_t value) {
  virtio_reg_write32(offset, virtio_reg_read32(offset) | value);
}

struct virtio_virtq *virtq_init(unsigned index) {
  paddr_t virtq_paddr = alloc_pages(align_up(sizeof(struct virtio_virtq), PAGE_SIZE) / PAGE_SIZE);
  struct virtio_virtq *vq = (struct virtio_virtq *) virtq_paddr;
  vq->queue_index = index;
  vq->used_index = (volatile uint16_t *) &vq->used.index;
  // select the queue writing its index to queuesel
  virtio_reg_write32(VIRTIO_REG_QUEUE_SEL, index);
  // checks the queue is not already in use
  uint32_t vq_pfn = virtio_reg_read32(VIRTIO_REG_QUEUE_PFN);
  if (vq_pfn == 0) {
    PANIC("virtio: invalid queue pfn");
  }
  // read max queue size
  uint32_t vq_size = virtio_reg_read32(VIRTIO_REG_QUEUE_NUM_MAX);
  if (vq_size == 0) {
    PANIC("virtio: invalid queue size");
  }
  // omitting the step 4
  // notify the device about the queue size
  virtio_reg_write32(VIRTIO_REG_QUEUE_NUM, VIRTQ_ENTRY_NUM);
  // notify the device about the used alignment
  virtio_reg_write32(VIRTIO_REG_QUEUE_ALIGN, 0x1000);
  // write the physical number of the first page of the queue
  virtio_reg_write32(VIRTIO_REG_QUEUE_PFN, virtq_paddr);
  return vq;
}

struct virtio_virtq *blk_request_vq;
struct virtio_blk_req *blk_req;
paddr_t blk_req_paddr;
unsigned blk_capacity;

void virtio_blk_init(void) {
  if (virtio_reg_read32(VIRTIO_REG_MAGIC) != 0x74726976)
    PANIC("virtio: invalid magic value");
  if (virtio_reg_read32(VIRTIO_REG_VERSION) != 1)
    PANIC("virtio: invalid version");
  if (virtio_reg_read32(VIRTIO_REG_DEVICE_ID) != VIRTIO_DEVICE_BLK)
    PANIC("virtio: invalid device id");
  
  // reset the device
  virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, 0);
  // set the ACKNOWLEDGE status bit
  virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_ACK);
  // set the DRIVER status bit
  virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER);
  // omitting step 4 at this point
  // set the FEATURES_OK status bit
  virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_FEAT_OK);
  // omitting step 6, which checks the device status bit to ensure the FEATURES_OK bit is still set
  // perform driver-specific setup
  blk_request_vq = virtq_init(0);
  // set the DRIVER_OK status bit
  virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER_OK);

  // get the capacity of the block device
  blk_capacity = virtio_reg_read64(VIRTIO_REG_DEVICE_CONFIG + 0) * SECTOR_SIZE;
  printf("virtio-blk: capacity=%d bytes\n", blk_capacity);

  // allocate a page for the request (1 virtio_blk_req)
  blk_req_paddr = alloc_pages(align_up(sizeof(*blk_req), PAGE_SIZE) / PAGE_SIZE);
  blk_req = (struct virtio_blk_req *) blk_req_paddr;
}

void virtq_kick(struct virtio_virtq *vq, int desc_index) {
  vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = desc_index;
  vq->avail.index++;
  __sync_synchronize();
  virtio_reg_write32(VIRTIO_REG_QUEUE_NOTIFY, vq->queue_index);
  vq->last_used_index++;
}

bool virtq_is_busy(struct virtio_virtq *vq) {
  return vq->last_used_index != *vq->used_index;
}

// user mode
extern char _binary_shell_bin_start[], _binary_shell_bin_size[];
__attribute__((naked))
void user_entry(void) {
  // set the user program counter to sepc
  // set the supervisor status to SPIE
  __asm__ __volatile__(
    "csrw sepc, %[sepc]\n"
    "csrw sstatus, %[sstatus]\n"
    "sret\n"
    :
    : [sepc] "r" (USER_BASE),
      [sstatus] "r" (SSTATUS_SPIE)
  );
}

extern char __kernel_base[];
// image: image of an application program
struct process *create_process(const void *image, size_t image_size) {
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
  *--sp = (uint32_t) user_entry;  // ra (return address)

  uint32_t *page_table = (uint32_t *) alloc_pages(1);

  // map the kernel memory to the process (__kernel_base ~ __free_ram_end)
  for (paddr_t paddr = (paddr_t) __kernel_base; paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE) {
    map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);
  }

  map_page(page_table, VIRTIO_BLK_PADDR, VIRTIO_BLK_PADDR, PAGE_R | PAGE_W);

  for (uint32_t off = 0; off < image_size; off += PAGE_SIZE) {
    paddr_t page = alloc_pages(1);

    // the last page may not be aligned to PAGE_SIZE
    size_t remaining = image_size - off;
    size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;

    memcpy((void *) page, image + off, copy_size);

    map_page(page_table, USER_BASE + off, page, PAGE_U | PAGE_R | PAGE_W | PAGE_X);
  }

  proc->pid = i + 1;
  proc->state = PROC_RUNNABLE;
  proc->sp = (uint32_t) sp;
  proc->page_table = page_table;
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
    "sfence.vma\n"
    "csrw satp, %[satp]\n"
    "sfence.vma\n"
    "csrw sscratch, %[sscratch]\n"
    :
    // sp address will be the end of next stack's address as stack is growing downwards
    : [satp] "r" (SATP_SV32 | ((uint32_t) next->page_table / PAGE_SIZE)),
      [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)])
  );

  struct process *prev = current_proc;
  current_proc = next;
  switch_context(&prev->sp, &next->sp);
}

void handle_syscall(struct trap_frame *f) {
  switch (f->a3) {
    case SYS_PUTCHAR:
      putchar(f->a0);
      break;
    case SYS_GETCHAR:
      // wait until a character is available
      while (1) {
        long ch = getchar();
        if (ch >= 0) {
          f->a0 = ch;
          break;
        }
        yield(); // yield to other processes for every loop because otherwise it will dominate the CPU
      }
      break;
    case SYS_EXIT:
      // in the actual OS, the page table and memory for this process should be freed
      printf("process %d exited\n", current_proc->pid);
      current_proc->state = PROC_EXITED;
      yield();
      PANIC("accessing exited process");
    default:
      PANIC("unknown syscall a3=%x\n", f->a3);
  }
}

void handle_trap(struct trap_frame *f) {
  uint32_t scause = READ_CSR(scause);
  uint32_t stval = READ_CSR(stval);
  uint32_t user_pc = READ_CSR(sepc);

  if (scause == SCAUSE_ECALL) {
    handle_syscall(f);
    user_pc += 4;
  } else {
    PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
  }

  WRITE_CSR(sepc, user_pc);
}

void kernel_main(void) {
  memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);
  
  WRITE_CSR(stvec, (uint32_t) kernel_entry);
  virtio_blk_init();

  idle_proc = create_process(NULL, 0);
  idle_proc->pid = -1;
  current_proc = idle_proc;

  create_process(_binary_shell_bin_start, (size_t) _binary_shell_bin_size);

  yield();

  PANIC("switched to idle process");
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
