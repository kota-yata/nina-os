#include "ipv4.h"
#include "icmp.h"
#include "../common.h"

size_t create_ipv4_packet(uint8_t *buffer, uint8_t src_ip[4], uint8_t dst_ip[4], uint8_t protocol, const uint8_t *payload, size_t payload_len) {
  struct ipv4_hdr *ip_hdr = (struct ipv4_hdr *)buffer;
  uint32_t src_ip_32 = address8to32(&src_ip);
  uint32_t dst_ip_32 = address8to32(&dst_ip);

  ip_hdr->version_ihl = (4 << 4) | (sizeof(struct ipv4_hdr) / 4);
  ip_hdr->type_of_service = 0;
  ip_hdr->total_length = htons(sizeof(struct ipv4_hdr) + payload_len);
  ip_hdr->identification = htons(1);
  ip_hdr->flags_fragment_offset = htons(0);
  ip_hdr->ttl = 64;
  ip_hdr->protocol = protocol;
  ip_hdr->header_checksum = 0;
  ip_hdr->src_ip = htonl(src_ip_32);
  ip_hdr->dst_ip = htonl(dst_ip_32);

  ip_hdr->header_checksum = calculate_checksum((uint16_t *)ip_hdr, sizeof(struct ipv4_hdr));

  memcpy(buffer + sizeof(struct ipv4_hdr), payload, payload_len);

  return sizeof(struct ipv4_hdr) + payload_len;
}


void handle_ipv4(struct ethernet_hdr *eth_hdr, struct ipv4_hdr *ip_hdr, uint32_t len) {
  if (ip_hdr->protocol == IPV4_PROTOCOL_ICMP) {
    handle_icmp(eth_hdr, ip_hdr, len);
  } else if (ip_hdr->protocol == IPV4_PROTOCOL_TCP) {
    printf("TCP packet received\n");
  } else if (ip_hdr->protocol == IPV4_PROTOCOL_UDP) {
    printf("UDP packet received\n");
  } else {
    printf("unknown ipv4 protocol %d\n", ip_hdr->protocol);
  }
}
