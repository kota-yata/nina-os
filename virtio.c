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
  // paddr_t virtq_paddr = alloc_pages(virtq_size / PAGE_SIZE);
  paddr_t virtq_paddr = alloc_pages(align_up(sizeof(struct virtio_virtq), PAGE_SIZE) / PAGE_SIZE);
  struct virtio_virtq *vq = (struct virtio_virtq *) virtq_paddr;
  // memset(vq, 0, sizeof(struct virtio_virtq));
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
  virtio_net_reg_write32(VIRTIO_REG_QUEUE_ALIGN, PAGE_SIZE);
  // write the physical number of the first page of the queue
  virtio_net_reg_write32(VIRTIO_REG_QUEUE_PFN, virtq_paddr / PAGE_SIZE);
  return vq;
}

// notification to the host driver
void virtq_kick(struct virtio_virtq *vq, int desc_index) {
  vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = desc_index;
  vq->avail.index++;
  __sync_synchronize();
  virtio_net_reg_write32(VIRTIO_REG_QUEUE_NOTIFY, vq->queue_index);
  vq->last_used_index++;
}

struct virtio_virtq *rx_vq;
struct virtio_virtq *tx_vq;

void virtio_net_qconfigure(void) {
  rx_vq = virtq_init(0);
  printf("virtio-net: descriptor size=%d\n", sizeof(struct virtq_desc));
  printf("virtio-net: avail size=%d\n", sizeof(struct virtq_avail));
  printf("virtio-net: used size=%d\n", sizeof(struct virtq_used));
  for (int i = 0; i < VIRTQ_ENTRY_NUM; i++) {
    paddr_t buf_paddr = alloc_pages(1);
    rx_vq->descs[i].addr = buf_paddr;
    rx_vq->descs[i].len = PAGE_SIZE;
    rx_vq->descs[i].flags = VIRTQ_DESC_F_WRITE;
    rx_vq->descs[i].next = (i + 1) % VIRTQ_ENTRY_NUM;
  }
  rx_vq->avail.index = 0;
  rx_vq->used_index = (volatile uint16_t *)&rx_vq->used.index;
  rx_vq->last_used_index = 0;
  // init tx_vq
  tx_vq = virtq_init(1);
  // for (int i = 0; i < VIRTQ_ENTRY_NUM; i++) {
  //   paddr_t buf_paddr = alloc_pages(1);
  //   // following fields will be populated during the actual transmission
  //   tx_vq->descs[i].addr = 0;
  //   tx_vq->descs[i].len = 0;
  //   tx_vq->descs[i].flags = 0;
  //   tx_vq->descs[i].next = (i + 1) % VIRTQ_ENTRY_NUM;
  // }
  tx_vq->avail.index = 0;
  tx_vq->used_index = (volatile uint16_t *)&tx_vq->used.index;
}

void virtio_net_init(void) {
  // step 1: Verify device magic value, version, and device ID
  if (virtio_net_reg_read32(VIRTIO_REG_MAGIC) != 0x74726976) {
    PANIC("virtio-net: invalid magic value");
  }
  // uint32_t version = virtio_net_reg_read32(VIRTIO_REG_VERSION);
  // if (version != 1) {
  //   PANIC("virtio-net: invalid version");
  // }
  if (virtio_net_reg_read32(VIRTIO_REG_DEVICE_ID) != VIRTIO_DEVICE_NET) {
    PANIC("virtio-net: invalid device id");
  }

  virtio_net_reg_write32(VIRTIO_REG_DEVICE_STATUS, 0);

  virtio_net_reg_write32(VIRTIO_REG_GUEST_PAGE_SIZE, PAGE_SIZE);

  virtio_net_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_ACK);
  virtio_net_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER);

  uint32_t guest_features = 0;
  // // Negotiate features (e.g., MAC address support)
  guest_features |= VIRTIO_NET_F_CSUM;
  guest_features |= VIRTIO_NET_F_MAC;
  virtio_net_reg_write32(VIRTIO_REG_DRIVER_FEATURES, guest_features);
  virtio_net_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_FEAT_OK);
  if (!(virtio_net_reg_read32(VIRTIO_REG_DEVICE_STATUS) & VIRTIO_STATUS_FEAT_OK)) {
    PANIC("virtio-net: feature negotiation failed");
  }
  virtio_net_qconfigure();
  virtio_net_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER_OK);

  // read MAC address from the device configuration space
  uint8_t mac[6];
  for (int i = 0; i < 6; i++) {
    mac[i] = *((volatile uint8_t *)(VIRTIO_NET_PADDR + VIRTIO_REG_DEVICE_CONFIG + i));
  }
  printf("virtio-net: MAC address: %x:%x:%x:%x:%x:%x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void virtio_net_handler(void) {
  struct virtio_virtq *vq = rx_vq; // RXキュー
  uint16_t last_used = vq->last_used_index;

  // ホストが使用したディスクリプタを確認
  while (last_used != *vq->used_index) {
    // UsedリングからディスクリプタIDを取得
    uint32_t desc_id = vq->used.ring[last_used % VIRTQ_ENTRY_NUM].id;

    // ディスクリプタからデータを取得
    struct virtio_net_hdr *hdr = (struct virtio_net_hdr *)(vq->descs[desc_id].addr);
    void *packet_data = (void *)(vq->descs[desc_id].addr + sizeof(struct virtio_net_hdr));
    uint32_t packet_len = vq->used.ring[last_used % VIRTQ_ENTRY_NUM].len - sizeof(struct virtio_net_hdr);

    // 上位プロトコルスタックに渡す（ここではログ出力）
    printf("Received packet: length=%d\n", packet_len);

    // ディスクリプタを再利用可能な状態に戻す
    vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = desc_id;
    vq->avail.index++;
    last_used++;
  }
  vq->last_used_index = last_used;
}

void virtio_net_transmit(void *data, uint32_t len) {
    struct virtio_virtq *vq = tx_vq;

    // descriptor ID
    uint16_t desc_id = vq->avail.index % VIRTQ_ENTRY_NUM;

    paddr_t packet_paddr = alloc_pages(1);

    struct virtio_net_hdr *hdr = (struct virtio_net_hdr *)packet_paddr;
    memset(hdr, 0, sizeof(struct virtio_net_hdr));

    void *packet_data = (void *)(packet_paddr + sizeof(struct virtio_net_hdr));
    memcpy(packet_data, data, len);

    vq->descs[desc_id].addr = packet_paddr;
    vq->descs[desc_id].len = sizeof(struct virtio_net_hdr) + len;
    vq->descs[desc_id].flags = 0;

    // add descriptor to the available ring
    vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = desc_id;
    vq->avail.index++;

    __sync_synchronize();
    virtio_net_reg_write32(VIRTIO_REG_QUEUE_NOTIFY, vq->queue_index);

    // wait until the packet is transmitted
    while (vq->last_used_index == *vq->used_index)
        ;

    return;
}

bool virtq_is_busy(struct virtio_virtq *vq) {
  return vq->last_used_index != *vq->used_index;
}

void debug_virtio_net(void) {
  uint32_t status = virtio_net_reg_read32(VIRTIO_REG_DEVICE_STATUS);
  
  if (status & VIRTIO_STATUS_ACK) {
      printf("virtio-net: Acknowledged\n");
  }
  if (status & VIRTIO_STATUS_DRIVER) {
      printf("virtio-net: Driver status set\n");
  }
  if (status & VIRTIO_STATUS_FEAT_OK) {
      printf("virtio-net: Features accepted\n");
  }
  if (status & VIRTIO_STATUS_DRIVER_OK) {
      printf("virtio-net: Driver ready\n");
  }
}
