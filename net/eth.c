#include "eth.h"
#include "../common.h"

size_t create_ethernet_frame(uint8_t *buffer, const uint8_t *src_mac, const uint8_t *dst_mac, uint16_t eth_type, const uint8_t *payload, size_t payload_len) {
  struct ethernet_hdr *eth_hdr = (struct ethernet_hdr *)buffer;

  memcpy(eth_hdr->src_mac, src_mac, 6);
  memcpy(eth_hdr->dst_mac, dst_mac, 6);
  eth_hdr->type = htons(eth_type);

  memcpy(buffer + sizeof(struct ethernet_hdr), payload, payload_len);

  return sizeof(struct ethernet_hdr) + payload_len;
}
