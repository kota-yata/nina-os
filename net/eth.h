#pragma once
#include "../common.h"

struct icmp_hdr {
  uint8_t type; // ICMP Type (8 for echo request, 0 for echo reply)
  uint8_t code; // ICMP Code (normally 0)
  uint16_t checksum;
  uint16_t identifier;
  uint16_t sequence;
} __attribute__((packed));

struct ethernet_hdr {
  uint8_t dst_mac[6];
  uint8_t src_mac[6];
  uint16_t type; // ETH_TYPE_*
} __attribute__((packed));

#define ETH_TYPE_ARP 0x0806
#define ETH_TYPE_IPV4 0x0800
