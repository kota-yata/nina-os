#include "../common.h"
#include "eth.h"
#include "arp.h"

void handle_arp_req(struct ethernet_hdr *req_eth_hdr, struct arp_payload *req_arp_payload){
  // return if src ip is different from my ip
  if (memcmp(req_arp_payload->target_ip, MY_IP_ADDRESS, sizeof(req_arp_payload->target_ip)) != 0) {
    printf("ARP request is not for me\n");
    return;
  }
  printf("ARP request is for me\n");
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
