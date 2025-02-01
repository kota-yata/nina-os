#pragma once
#include "../common.h"
#include "../virtio.h"

#define VIRTIO_NET_PADDR 0x10001000
#define VIRTIO_NET_MAX_PACKET_SIZE 1514 // see https://docs.oasis-open.org/virtio/virtio/v1.1/csprd01/virtio-v1.1-csprd01.html#x1-1560004:~:text=headers%20and%20packets.-,5.1.6.3%20Setting%20Up%20Receive%20Buffers,-It%20is%20generally
#define VIRTIO_NET_F_MAC (1 << 5)
#define VIRTIO_NET_F_CSUM (1 << 0)

struct virtio_net_hdr {
  uint8_t flags;
  uint8_t gso_type;
  uint16_t hdr_len;
  uint16_t gso_size;
  uint16_t csum_start;
  uint16_t csum_offset;
} __attribute__((packed));

struct virtio_net_req {
  struct virtio_net_hdr header;
  uint8_t payload[VIRTIO_NET_MAX_PACKET_SIZE];
} __attribute__((packed));

struct virtio_net_ctrl_hdr {
  uint8_t cls;
  uint8_t cmd;
} __attribute__((packed));

void virtio_net_init(void);
void debug_virtio_net(void);
void virtio_net_handler(void);
void virtio_net_transmit(void *data, size_t len);
