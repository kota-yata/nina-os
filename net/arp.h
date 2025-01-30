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

#define ARP_TABLE_SIZE 16
#define IP_ADDR_SIZE 4
#define MAC_ADDR_SIZE 6

struct arp_entry {
  uint8_t ip[IP_ADDR_SIZE];
  uint8_t mac[MAC_ADDR_SIZE];
  bool valid;
};

static struct arp_entry arp_table[ARP_TABLE_SIZE];

void handle_arp_req(struct ethernet_hdr *req_eth_hdr, struct arp_payload *req_arp_payload);
void send_arp_request(uint8_t target_ip[IP_ADDR_SIZE]);
void handle_arp(struct ethernet_hdr *eth_hdr, struct arp_payload *arp_payload);
