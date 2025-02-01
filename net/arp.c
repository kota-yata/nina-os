#include "../common.h"
#include "eth.h"
#include "arp.h"
#include "dpq.h"
#include "virtio.h"

struct arp_entry *arp_lookup(uint8_t ip[IP_ADDR_SIZE]) {
  for (int i = 0; i < ARP_TABLE_SIZE; i++) {
    if (arp_table[i].valid && memcmp(arp_table[i].ip, ip, IP_ADDR_SIZE) == 0) {
      printf("ARP entry found\n");
      return &arp_table[i];
    }
  }
  return NULL;
}

void arp_update(uint8_t ip[IP_ADDR_SIZE], uint8_t mac[MAC_ADDR_SIZE]) {
  struct arp_entry *entry = arp_lookup(ip);
  if (entry) {
    memcpy(entry->mac, mac, MAC_ADDR_SIZE);
  } else {
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
      if (!arp_table[i].valid) {
        memcpy(arp_table[i].ip, ip, IP_ADDR_SIZE);
        memcpy(arp_table[i].mac, mac, MAC_ADDR_SIZE);
        arp_table[i].valid = true;
        return;
      }
    }
    printf("ARP table is full\n");
  }
}

void arp_delete(uint8_t ip[IP_ADDR_SIZE]) {
  struct arp_entry *entry = arp_lookup(ip);
  if (entry) {
    entry->valid = false;
  }
}

void print_arp_table(void) {
  printf("ARP Table:\n");
  for (int i = 0; i < ARP_TABLE_SIZE; i++) {
    if (arp_table[i].valid) {
      printf("IP: %d.%d.%d.%d -> MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        arp_table[i].ip[0], arp_table[i].ip[1], arp_table[i].ip[2], arp_table[i].ip[3],
        arp_table[i].mac[0], arp_table[i].mac[1], arp_table[i].mac[2],
        arp_table[i].mac[3], arp_table[i].mac[4], arp_table[i].mac[5]);
    }
  }
}


void handle_arp_req(struct ethernet_hdr *req_eth_hdr, struct arp_payload *req_arp_payload){
  // return if src ip is different from my ip
  if (memcmp(req_arp_payload->target_ip, MY_IP_ADDRESS, sizeof(req_arp_payload->target_ip)) != 0) {
    printf("ARP request is not for me\n");
    return;
  }
  printf("Updating ARP table with incoming ARP reply\n");
  // update arp table
  arp_update(req_arp_payload->sender_ip, req_arp_payload->sender_mac);

  // send ARP reply
  struct ethernet_hdr *resp_eth_hdr = (struct ethernet_hdr *)req_eth_hdr;
  struct arp_payload *resp_arp = req_arp_payload;

  memcpy(resp_eth_hdr->dst_mac, req_eth_hdr->src_mac, sizeof(resp_eth_hdr->dst_mac));
  memcpy(resp_eth_hdr->src_mac, MY_MAC_ADDRESS, sizeof(resp_eth_hdr->src_mac));
  resp_eth_hdr->type = htons(ETH_TYPE_ARP);

  resp_arp->hw_type = htons(ARP_HW_TYPE_ETH);
  resp_arp->proto_type = htons(ETH_TYPE_IPV4);
  resp_arp->hw_addr_len = 6;
  resp_arp->proto_addr_len = 4;
  resp_arp->opcode = htons(ARP_OP_REPLY);
  memcpy(resp_arp->target_mac, req_arp_payload->sender_mac, sizeof(resp_arp->target_mac));
  memcpy(resp_arp->target_ip, req_arp_payload->sender_ip, sizeof(resp_arp->target_ip));
  memcpy(resp_arp->sender_mac, MY_MAC_ADDRESS, sizeof(resp_arp->sender_mac));
  memcpy(resp_arp->sender_ip, MY_IP_ADDRESS, sizeof(resp_arp->sender_ip));
  printf("Sending ARP reply to %d.%d.%d.%d\n", resp_arp->target_ip[0], resp_arp->target_ip[1], resp_arp->target_ip[2], resp_arp->target_ip[3]);

  void *packet_data = (void *)resp_eth_hdr;

  virtio_net_transmit(packet_data, sizeof(struct ethernet_hdr) + sizeof(struct arp_payload));
};

void send_arp_request(uint8_t target_ip[IP_ADDR_SIZE]) {
  uint8_t packet[sizeof(struct ethernet_hdr) + sizeof(struct arp_payload)];
  struct ethernet_hdr *eth_hdr = (struct ethernet_hdr *)packet;
  struct arp_payload *arp = (struct arp_payload *)(eth_hdr + 1);

  memset(eth_hdr->dst_mac, 0xff, sizeof(eth_hdr->dst_mac));
  memcpy(eth_hdr->src_mac, MY_MAC_ADDRESS, sizeof(eth_hdr->src_mac));
  eth_hdr->type = htons(ETH_TYPE_ARP);

  arp->hw_type = htons(ARP_HW_TYPE_ETH);
  arp->proto_type = htons(ETH_TYPE_IPV4);
  arp->hw_addr_len = 6;
  arp->proto_addr_len = 4;
  arp->opcode = htons(ARP_OP_REQUEST);
  memcpy(arp->sender_mac, MY_MAC_ADDRESS, sizeof(arp->sender_mac));
  memcpy(arp->sender_ip, MY_IP_ADDRESS, sizeof(arp->sender_ip));
  memset(arp->target_mac, 0, sizeof(arp->target_mac));
  memcpy(arp->target_ip, target_ip, sizeof(arp->target_ip));

  void *packet_data = (void *)eth_hdr;
  printf("Sending ARP request to %d.%d.%d.%d\n", target_ip[0], target_ip[1], target_ip[2], target_ip[3]);

  virtio_net_transmit(packet_data, sizeof(struct ethernet_hdr) + sizeof(struct arp_payload));
}

void handle_arp_reply(struct ethernet_hdr *eth_hdr, struct arp_payload *arp) {
  printf("Received ARP reply\n");
  arp_update(arp->sender_ip, arp->sender_mac);
  // process deferred packets
  uint8_t *dst_ip_address = arp->sender_ip;
  const uint32_t dst_ip_address_u32 =
      ((uint32_t)dst_ip_address[0] << 24) |
      ((uint32_t)dst_ip_address[1] << 16) |
      ((uint32_t)dst_ip_address[2] << 8) |
      ((uint32_t)dst_ip_address[3]);
  process_deferred_packets(dst_ip_address_u32, arp->sender_mac);
}

void handle_arp(struct ethernet_hdr *eth_hdr, struct arp_payload *arp) {
  if (ntohs(arp->opcode) == ARP_OP_REQUEST) {
    handle_arp_req(eth_hdr, arp);
  } else if (ntohs(arp->opcode) == ARP_OP_REPLY) {
    handle_arp_reply(eth_hdr, arp);
  } else {
    printf("Unknown ARP opcode %d\n", ntohs(arp->opcode));
  }
}
