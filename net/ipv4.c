#include "ipv4.h"
#include "icmp.h"

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
