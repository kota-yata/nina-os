#pragma once
#include "../common.h"

struct ethernet_hdr {
  uint8_t dst_mac[6];
  uint8_t src_mac[6];
  uint16_t type; // ETH_TYPE_*
} __attribute__((packed));

#define ETH_TYPE_ARP 0x0806
#define ETH_TYPE_IPV4 0x0800
