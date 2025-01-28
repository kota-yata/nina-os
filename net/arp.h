#pragma once
#include "eth.h"
#include "../common.h"

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

#define ARP_HW_TYPE_ETH 1
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY 2

void handle_arp_req(struct ethernet_hdr *req_eth_hdr, struct arp_payload *req_arp_payload);
