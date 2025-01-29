#pragma once
#include "../common.h"
#include "eth.h"

struct ipv4_hdr {
  uint8_t version_ihl;
  uint8_t type_of_service;
  uint16_t total_length;
  uint16_t identification;
  uint16_t flags_fragment_offset;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t header_checksum;
  uint32_t src_ip;
  uint32_t dst_ip;
} __attribute__((packed));

#define IPV4_PROTOCOL_ICMP 1
#define IPV4_PROTOCOL_TCP 6
#define IPV4_PROTOCOL_UDP 17

void handle_ipv4(struct ethernet_hdr *eth_hdr, struct ipv4_hdr *ip_hdr, uint32_t len);
