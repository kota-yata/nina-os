#pragma once
#include "../common.h"

struct ethernet_hdr {
  uint8_t dst_mac[6];
  uint8_t src_mac[6];
  uint16_t type; // ETH_TYPE_*
} __attribute__((packed));

#define ETH_TYPE_ARP 0x0806
#define ETH_TYPE_IPV4 0x0800

size_t create_ethernet_frame(uint8_t *buffer, const uint8_t *src_mac, const uint8_t *dst_mac, uint16_t eth_type, const uint8_t *payload, size_t payload_len);
