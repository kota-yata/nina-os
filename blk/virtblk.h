#pragma once
#include "../common.h"

#define SECTOR_SIZE 512
#define VIRTIO_BLK_PADDR 0x10002000
#define VIRTIO_BLK_T_IN  0
#define VIRTIO_BLK_T_OUT 1

struct virtio_blk_req {
  uint32_t type;
  uint32_t reserved;
  uint64_t sector;
  uint8_t data[512];
  uint8_t status;
} __attribute__((packed));

void virtio_blk_init(void);
void read_write_disk(void *buf, unsigned sector, int is_write);
