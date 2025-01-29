#pragma once
#include "eth.h"
#include "ipv4.h"

struct icmp_hdr {
  uint8_t type; // ICMP Type (8 for echo request, 0 for echo reply)
  uint8_t code; // ICMP Code (normally 0)
  uint16_t checksum;
  uint16_t identifier;
  uint16_t sequence;
} __attribute__((packed));

#define ICMP_TYPE_ECHO_REPLY 0
#define ICMP_TYPE_ECHO_REQUEST 8

void send_icmp_echo_request();
void handle_icmp(struct ethernet_hdr *eth_hdr, struct ipv4_hdr *ip_hdr, uint32_t len);
