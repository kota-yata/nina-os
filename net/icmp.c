#include "virtio.h"
#include "eth.h"
#include "ipv4.h"
#include "icmp.h"
#include "../common.h"

uint16_t calculate_checksum(uint16_t *header, int length) {
    unsigned long sum = 0;
    while (length > 1) {
      sum += *header++;
      length -= 2;
    }
    // in case of odd number of bytes
    if (length == 1) {
      sum += *(uint8_t *)header;
    }
    // add carry
    while (sum >> 16) {
      sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (uint16_t)(~sum);
}


const char payload[9] = "PING TEST";
const uint32_t dst_ip_address = 0xC0A86465; // 192.168.100.101
// const uint8_t dst_mac_address[6] = {0x50, 0x6b, 0x4b, 0x08, 0x61, 0xde};
const uint8_t dst_mac_address[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

void send_icmp_echo_request() {
  printf("Sending ICMP Echo Request to 192.168.100.101...\n");

  // packet buffer
  uint8_t packet[128];
  
  struct ethernet_hdr *eth_hdr = (struct ethernet_hdr *)packet;
  struct ipv4_hdr *ip_hdr = (struct ipv4_hdr *)(packet + sizeof(struct ethernet_hdr));
  struct icmp_hdr *icmp_hdr = (struct icmp_hdr *)(packet + sizeof(struct ethernet_hdr) + sizeof(struct ipv4_hdr));

  memcpy(eth_hdr->dst_mac, dst_mac_address, 6);
  memcpy(eth_hdr->src_mac, MY_MAC_ADDRESS, 6);
  eth_hdr->type = htons(0x0800); // IPv4

  ip_hdr->version_ihl = (4 << 4) | (sizeof(struct ipv4_hdr) / 4); // IPv4, 20 bytes
  ip_hdr->type_of_service = 0;
  ip_hdr->total_length = htons(sizeof(struct ipv4_hdr) + sizeof(struct icmp_hdr) + sizeof(payload));
  ip_hdr->identification = htons(1);
  ip_hdr->flags_fragment_offset = htons(0);
  ip_hdr->ttl = 64;
  ip_hdr->protocol = 1; // icmp
  ip_hdr->header_checksum = 0;
  ip_hdr->src_ip = htonl(MY_IP_ADDRESS_32);
  ip_hdr->dst_ip = htonl(dst_ip_address);
  ip_hdr->header_checksum = calculate_checksum((uint16_t *)ip_hdr, sizeof(struct ipv4_hdr));

  icmp_hdr->type = ICMP_TYPE_ECHO_REQUEST;
  icmp_hdr->code = 0;
  icmp_hdr->checksum = 0;
  icmp_hdr->identifier = htons(12345);
  icmp_hdr->sequence = htons(1);

  // arbitrary data for payload
  size_t hdr_len = sizeof(struct ethernet_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct icmp_hdr);
  char *data = (char *)(packet + hdr_len);
  memcpy(data, payload, sizeof(payload));

  icmp_hdr->checksum = calculate_checksum((uint16_t *)icmp_hdr, sizeof(struct icmp_hdr) + sizeof(payload));

  size_t packet_size = hdr_len + sizeof(payload);
  virtio_net_transmit(packet, sizeof(packet));

  printf("ICMP Echo Request sent\n");
}

void handle_icmp_echo_request(struct ethernet_hdr *req_eth_hdr, struct ipv4_hdr *req_ip_hdr, struct icmp_hdr *req_icmp_hdr) {
  printf("Received ICMP Echo Request\n");

  struct ethernet_hdr *resp_eth_hdr = (struct ethernet_hdr *)req_eth_hdr;
  struct ipv4_hdr *resp_ip_hdr = (struct ipv4_hdr *)(req_eth_hdr + 1);
  struct icmp_hdr *resp_icmp_hdr = (struct icmp_hdr *)(resp_ip_hdr + 1);
  size_t payload_len = ntohs(req_ip_hdr->total_length) - sizeof(struct ipv4_hdr) - sizeof(struct icmp_hdr);
  printf("Payload length: %d\n", payload_len);

  memcpy(resp_eth_hdr->dst_mac, req_eth_hdr->src_mac, 6);
  memcpy(resp_eth_hdr->src_mac, MY_MAC_ADDRESS, 6);
  resp_eth_hdr->type = htons(ETH_TYPE_IPV4);

  resp_ip_hdr->dst_ip = req_ip_hdr->src_ip;
  resp_ip_hdr->src_ip = htonl(MY_IP_ADDRESS_32);

  resp_icmp_hdr->type = ICMP_TYPE_ECHO_REPLY;
  resp_icmp_hdr->code = 0;
  resp_icmp_hdr->checksum = 0;
  resp_icmp_hdr->checksum = calculate_checksum((uint16_t *)resp_icmp_hdr, sizeof(struct icmp_hdr) + payload_len);

  void *packet_data = (void *)resp_eth_hdr;
  size_t packet_len = ntohs(req_ip_hdr->total_length);
  virtio_net_transmit(packet_data, packet_len + sizeof(struct ethernet_hdr));
}

void handle_icmp(struct ethernet_hdr *eth_hdr, struct ipv4_hdr *ip_hdr, uint32_t len) {
  struct icmp_hdr *icmp_hdr = (struct icmp_hdr *)(ip_hdr + 1);
  if (icmp_hdr->type == ICMP_TYPE_ECHO_REQUEST) {
    handle_icmp_echo_request(eth_hdr, ip_hdr, icmp_hdr);
  } else if (icmp_hdr->type == ICMP_TYPE_ECHO_REPLY) {
    printf("Received ICMP Echo Reply\n");
  } else {
    printf("Unknown ICMP type %d\n", icmp_hdr->type);
  }
}
