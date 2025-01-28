#pragma once
#include "../common.h"

struct icmp_header {
  uint8_t type; // ICMP Type (8 for echo request, 0 for echo reply)
  uint8_t code; // ICMP Code (normally 0)
  uint16_t checksum;
  uint16_t identifier;
  uint16_t sequence;
} __attribute__((packed));

struct ipv4_header {
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

struct arp_payload {
  uint16_t hw_type;
  uint16_t proto_type;
  uint8_t hw_addr_len;
  uint8_t proto_addr_len;
  uint16_t opcode;
  uint8_t sender_mac[6];
  uint8_t sender_ip[4];
  uint8_t target_mac[6];
  uint8_t target_ip[4];
} __attribute__((packed));

struct ethernet_header {
  uint8_t dst_mac[6];
  uint8_t src_mac[6];
  uint16_t type; // IPv4: 0x0800
} __attribute__((packed));

void send_icmp_echo_request();
