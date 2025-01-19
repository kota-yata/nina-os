#include "virtio.h"
#include "kernel.h"
#include "common.h"

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

  // read MAC address from the device configuration space
  uint8_t mac[6];
  for (int i = 0; i < 6; i++) {
    mac[i] = *((volatile uint8_t *)(VIRTIO_NET_PADDR + VIRTIO_REG_DEVICE_CONFIG + i));
  }
  printf("virtio-net: MAC address: %x:%x:%x:%x:%x:%x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  virtio_net_qconfigure();

  virtio_net_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER_OK);
}

void virtio_net_interrupt_handler(void) {
  while(*current_virtio_net.rx_vq->used_index != current_virtio_net.rx_vq->last_used_index) {
    printf("virtio-net: packet received\n");
    // uint16_t desc_index = current_virtio_net.rx_vq->used.ring[current_virtio_net.rx_vq->last_used_index % VIRTQ_ENTRY_NUM].id;
    // struct virtq_desc *desc = &current_virtio_net.rx_vq->descs[desc_index];
  }
}

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
