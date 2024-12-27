#include "common.h"

void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags){
  if (!is_aligned(vaddr, PAGE_SIZE)) {
    PANIC("unalined vaddr %x", vaddr);
  }
  if (!is_aligned(paddr, PAGE_SIZE)) {
    PANIC("unalined paddr %x", paddr);
  }
}

