#include "virtio.h"
#include "kernel.h"
#include "common.h"

// virtio
// uint32_t virtio_blk_reg_read32(unsigned offset) {
//   return *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset));
// }
// uint64_t virtio_blk_reg_read64(unsigned offset) {
//   return *((volatile uint64_t *) (VIRTIO_BLK_PADDR + offset));
// }
// void virtio_blk_reg_write32(unsigned offset, uint32_t value) {
//   *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset)) = value;
// }
// void virtio_blk_reg_fetch_and_or32(unsigned offset, uint32_t value) {
//   virtio_blk_reg_write32(offset, virtio_blk_reg_read32(offset) | value);
// }

uint32_t virtio_net_reg_read32(unsigned offset) {
    return *((volatile uint32_t *)(VIRTIO_NET_PADDR + offset));
}
void virtio_net_reg_write32(unsigned offset, uint32_t value) {
    *((volatile uint32_t *)(VIRTIO_NET_PADDR + offset)) = value;
}
void virtio_net_reg_fetch_and_or32(unsigned offset, uint32_t value) {
  virtio_net_reg_write32(offset, virtio_net_reg_read32(offset) | value);
}

struct virtio_virtq *virtq_init(unsigned index) {
  paddr_t virtq_paddr = alloc_pages(align_up(sizeof(struct virtio_virtq), PAGE_SIZE) / PAGE_SIZE);
  struct virtio_virtq *vq = (struct virtio_virtq *) virtq_paddr;
  vq->queue_index = index;
  vq->used_index = (volatile uint16_t *) &vq->used.index;
  // select the queue writing its index to queuesel
  virtio_net_reg_write32(VIRTIO_REG_QUEUE_SEL, index);
  // checks the queue is not already in use
  uint32_t vq_pfn = virtio_net_reg_read32(VIRTIO_REG_QUEUE_PFN);
  if (vq_pfn != 0) {
    PANIC("virtio: invalid queue pfn");
  }
  // read max queue size
  uint32_t vq_size = virtio_net_reg_read32(VIRTIO_REG_QUEUE_NUM_MAX);
  if (vq_size == 0) {
    PANIC("virtio: invalid queue size");
  }
  // omitting the step 4
  // notify the device about the queue size
  virtio_net_reg_write32(VIRTIO_REG_QUEUE_NUM, VIRTQ_ENTRY_NUM);
  // notify the device about the used alignment
  virtio_net_reg_write32(VIRTIO_REG_QUEUE_ALIGN, 0x1000);
  // write the physical number of the first page of the queue
  virtio_net_reg_write32(VIRTIO_REG_QUEUE_PFN, virtq_paddr);
  return vq;
}

struct virtio_net_curr current_virtio_net;

void virtio_net_qconfigure(void) {
  // init rx_vq
  current_virtio_net.rx_vq = virtq_init(0);
  for (int i = 0; i < VIRTQ_ENTRY_NUM; i++) {
    paddr_t buf_paddr = alloc_pages(1);
    current_virtio_net.rx_vq->descs[i].addr = buf_paddr;
    current_virtio_net.rx_vq->descs[i].len = PAGE_SIZE;
    current_virtio_net.rx_vq->descs[i].flags = VIRTQ_DESC_F_WRITE;
    current_virtio_net.rx_vq->descs[i].next = (i + 1) % VIRTQ_ENTRY_NUM;
  }
  current_virtio_net.rx_vq->avail.index = 0;
  current_virtio_net.rx_vq->used_index = (volatile uint16_t *)&current_virtio_net.rx_vq->used.index;
  // init tx_vq
  current_virtio_net.tx_vq = virtq_init(1);
  for (int i = 0; i < VIRTQ_ENTRY_NUM; i++) {
    paddr_t buf_paddr = alloc_pages(1);
    // following fields will be populated during the actual transmission
    current_virtio_net.tx_vq->descs[i].addr = 0;
    current_virtio_net.tx_vq->descs[i].len = 0;
    current_virtio_net.tx_vq->descs[i].flags = 0;
    current_virtio_net.tx_vq->descs[i].next = (i + 1) % VIRTQ_ENTRY_NUM;
  }
  current_virtio_net.tx_vq->avail.index = 0;
  current_virtio_net.tx_vq->used_index = (volatile uint16_t *)&current_virtio_net.tx_vq->used.index;
}

void virtio_net_init(void) {
  // step 1: Verify device magic value, version, and device ID
  printf("VirtIO Magic: 0x%x\n", virtio_net_reg_read32(VIRTIO_REG_MAGIC));
  if (virtio_net_reg_read32(VIRTIO_REG_MAGIC) != 0x74726976) {
      PANIC("virtio-net: invalid magic value");
  }
  printf("VirtIO Version: %d\n", virtio_net_reg_read32(VIRTIO_REG_VERSION));
  if (virtio_net_reg_read32(VIRTIO_REG_VERSION) != 1) {
      PANIC("virtio-net: invalid version");
  }
  printf("VirtIO Device ID: %d\n", virtio_net_reg_read32(VIRTIO_REG_DEVICE_ID));
  if (virtio_net_reg_read32(VIRTIO_REG_DEVICE_ID) != VIRTIO_DEVICE_NET) {
      PANIC("virtio-net: invalid device id");
  }

  // Step 2: Reset the device
  virtio_net_reg_write32(VIRTIO_REG_DEVICE_STATUS, 0);

  // Step 3: Acknowledge and initialize
  virtio_net_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_ACK);
  virtio_net_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER);

  // uint32_t host_features = virtio_blk_reg_read32(VIRTIO_REG_DEVICE_FEATURES);
  // uint32_t guest_features = 0;

  // // Negotiate features (e.g., MAC address support)
  // if (host_features & (1 << VIRTIO_NET_F_MAC)) {
  //     guest_features |= (1 << VIRTIO_NET_F_MAC);
  // }
  // virtio_blk_reg_write32(VIRTIO_REG_DRIVER_FEATURES, guest_features)
  virtio_net_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_FEAT_OK);
  if (!(virtio_net_reg_read32(VIRTIO_REG_DEVICE_STATUS) & VIRTIO_STATUS_FEAT_OK)) {
    PANIC("virtio-net: feature negotiation failed");
  }
  virtio_net_qconfigure();

  virtio_net_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER_OK);
}

struct virtio_virtq *blk_request_vq;
struct virtio_blk_req *blk_req;
paddr_t blk_req_paddr;
unsigned blk_capacity;

// void virtio_blk_init(void) {
//   if (virtio_blk_reg_read32(VIRTIO_REG_MAGIC) != 0x74726976)
//     PANIC("virtio: invalid magic value");
//   if (virtio_blk_reg_read32(VIRTIO_REG_VERSION) != 1)
//     PANIC("virtio: invalid version");
//   if (virtio_blk_reg_read32(VIRTIO_REG_DEVICE_ID) != VIRTIO_DEVICE_BLK)
//     PANIC("virtio: invalid device id");
  
//   // reset the device
//   virtio_blk_reg_write32(VIRTIO_REG_DEVICE_STATUS, 0);
//   // set the ACKNOWLEDGE status bit
//   virtio_blk_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_ACK);
//   // set the DRIVER status bit
//   virtio_blk_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER);
//   // omitting step 4 at this point
//   // set the FEATURES_OK status bit
//   virtio_blk_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_FEAT_OK);
//   // omitting step 6, which checks the device status bit to ensure the FEATURES_OK bit is still set
//   // perform driver-specific setup
//   blk_request_vq = virtq_init(0);
//   // set the DRIVER_OK status bit
//   virtio_blk_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER_OK);

//   // get the capacity of the block device
//   blk_capacity = virtio_blk_reg_read64(VIRTIO_REG_DEVICE_CONFIG + 0) * SECTOR_SIZE;
//   printf("virtio-blk: capacity=%d bytes\n", blk_capacity);

//   // allocate a page for the request (1 virtio_blk_req)
//   blk_req_paddr = alloc_pages(align_up(sizeof(*blk_req), PAGE_SIZE) / PAGE_SIZE);
//   blk_req = (struct virtio_blk_req *) blk_req_paddr;
// }

// notification to the host driver
void virtq_kick(struct virtio_virtq *vq, int desc_index) {
  vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = desc_index;
  vq->avail.index++;
  __sync_synchronize();
  virtio_net_reg_write32(VIRTIO_REG_QUEUE_NOTIFY, vq->queue_index);
  vq->last_used_index++;
}

bool virtq_is_busy(struct virtio_virtq *vq) {
  return vq->last_used_index != *vq->used_index;
}

void read_write_disk(void *buf, unsigned sector, int is_write) {
  if (sector >= blk_capacity / SECTOR_SIZE) {
    PANIC("virtio-blk: out of capacity %d", sector);
  }
  blk_req->sector = sector;
  blk_req->type = is_write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
  if (is_write) {
    memcpy(blk_req->data, buf, SECTOR_SIZE);
  }
  struct virtio_virtq *vq = blk_request_vq;
  vq->descs[0].addr = blk_req_paddr;
  vq->descs[0].len = sizeof(uint32_t) * 2 + sizeof(uint64_t);
  vq->descs[0].flags = VIRTQ_DESC_F_NEXT;
  vq->descs[0].next = 1;

  vq->descs[1].addr = blk_req_paddr + offsetof(struct virtio_blk_req, data);
  vq->descs[1].len = SECTOR_SIZE;
  vq->descs[1].flags = VIRTQ_DESC_F_NEXT | (is_write ? 0 : VIRTQ_DESC_F_WRITE);
  vq->descs[1].next = 2;

  vq->descs[2].addr = blk_req_paddr + offsetof(struct virtio_blk_req, status);
  vq->descs[2].len = sizeof(uint8_t);
  vq->descs[2].flags = VIRTQ_DESC_F_WRITE;

  virtq_kick(vq, 0);
  while (virtq_is_busy(vq)) {
    ;
  }
  if (blk_req->status != 0) {
    PANIC("virtio-blk: failed to %s sector %d", is_write ? "write" : "read", sector);
  }
  if (!is_write) {
    memcpy(buf, blk_req->data, SECTOR_SIZE);
  }
}

void debug_virtio_net(void) {
  if (!current_virtio_net.rx_vq || !current_virtio_net.tx_vq) {
    PANIC("virtio-net: queues not configured");
  }
  printf("virtio-net: rx_vq=%x, tx_vq=%x\n", current_virtio_net.rx_vq, current_virtio_net.tx_vq);
  for (int i = 0; i < VIRTQ_ENTRY_NUM; i++) {
    if (current_virtio_net.rx_vq->descs[i].addr == 0 || current_virtio_net.rx_vq->descs[i].len == 0) {
      PANIC("virtio-net: rx_vq[%d] not configured properly", i);
    }
    printf("virtio-net: rx_vq[%d] addr=%x, len=%d\n", i, current_virtio_net.rx_vq->descs[i].addr, current_virtio_net.rx_vq->descs[i].len);
  }
  for (int i = 0; i < VIRTQ_ENTRY_NUM; i++) {
    printf("virtio-net: tx_vq[%d] addr=%x, len=%d\n", i, current_virtio_net.tx_vq->descs[i].addr, current_virtio_net.tx_vq->descs[i].len);
  }
}
