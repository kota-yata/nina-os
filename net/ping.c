#include "virtio.h"
#include "ping.h"

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
const uint32_t src_ip_address = 0xC0A86468; // 192.168.100.104
const uint32_t dst_ip_address = 0xC0A86465; // 192.168.100.101
const uint8_t src_mac_address[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
// const uint8_t dst_mac_address[6] = {0x50, 0x6b, 0x4b, 0x08, 0x61, 0xde};
const uint8_t dst_mac_address[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

void send_icmp_echo_request() {
  printf("Sending ICMP Echo Request to 192.168.100.101...\n");

  // packet buffer
  uint8_t packet[128];
  
  struct ethernet_header *eth_hdr = (struct ethernet_header *)packet;
  struct ipv4_header *ip_hdr = (struct ipv4_header *)(packet + sizeof(struct ethernet_header));
  struct icmp_header *icmp_hdr = (struct icmp_header *)(packet + sizeof(struct ethernet_header) + sizeof(struct ipv4_header));

  memcpy(eth_hdr->dst_mac, dst_mac_address, 6);
  memcpy(eth_hdr->src_mac, src_mac_address, 6);
  eth_hdr->type = htons(0x0800); // IPv4

  ip_hdr->version_ihl = (4 << 4) | (sizeof(struct ipv4_header) / 4); // IPv4, 20 bytes
  ip_hdr->type_of_service = 0;
  ip_hdr->total_length = htons(sizeof(struct ipv4_header) + sizeof(struct icmp_header) + sizeof(payload));
  ip_hdr->identification = htons(1);
  ip_hdr->flags_fragment_offset = htons(0);
  ip_hdr->ttl = 64;
  ip_hdr->protocol = 1; // icmp
  ip_hdr->header_checksum = 0;
  ip_hdr->src_ip = htonl(src_ip_address);
  ip_hdr->dst_ip = htonl(dst_ip_address);
  ip_hdr->header_checksum = calculate_checksum((uint16_t *)ip_hdr, sizeof(struct ipv4_header));

  icmp_hdr->type = 8;
  icmp_hdr->code = 0;
  icmp_hdr->checksum = 0;
  icmp_hdr->identifier = htons(12345);
  icmp_hdr->sequence = htons(1);

  // arbitrary data for payload
  size_t hdr_len = sizeof(struct ethernet_header) + sizeof(struct ipv4_header) + sizeof(struct icmp_header);
  char *data = (char *)(packet + hdr_len);
  memcpy(data, payload, sizeof(payload));

  icmp_hdr->checksum = calculate_checksum((uint16_t *)icmp_hdr, sizeof(struct icmp_header) + sizeof(payload));

  size_t packet_size = hdr_len + sizeof(payload);
  virtio_net_transmit(packet, sizeof(packet));

  printf("ICMP Echo Request sent\n");
}
