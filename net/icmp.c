#include "virtio.h"
#include "eth.h"
#include "ipv4.h"
#include "arp.h"
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
const uint8_t dst_ip_address[4] = {192, 168, 100, 101};

size_t create_icmp_packet(uint8_t *buffer, uint8_t type, uint8_t code, uint16_t identifier, uint16_t sequence, const char *payload, size_t payload_len) {
  struct icmp_hdr *icmp_hdr = (struct icmp_hdr *)buffer;

  icmp_hdr->type = type;
  icmp_hdr->code = code;
  icmp_hdr->checksum = 0;
  icmp_hdr->identifier = htons(identifier);
  icmp_hdr->sequence = htons(sequence);

  memcpy(buffer + sizeof(struct icmp_hdr), payload, payload_len);

  // チェックサム計算
  size_t total_len = sizeof(struct icmp_hdr) + payload_len;
  icmp_hdr->checksum = calculate_checksum((uint16_t *)buffer, total_len);

  return total_len;
}

size_t prepare_icmp_req(uint8_t *packet, const uint8_t *dst_ip_address) {
  struct ethernet_hdr *eth_hdr = (struct ethernet_hdr *)packet;
  struct ipv4_hdr *ipv4_hdr = (struct ipv4_hdr *)(packet + sizeof(struct ethernet_hdr));
  struct icmp_hdr *icmp_hdr = (struct icmp_hdr *)(packet + sizeof(struct ethernet_hdr) + sizeof(struct ipv4_hdr));
  size_t icmp_len = create_icmp_packet((uint8_t *)icmp_hdr, ICMP_TYPE_ECHO_REQUEST, 0, 12345, 1, payload, sizeof(payload));
  size_t ipv4_len = create_ipv4_packet((uint8_t *)ipv4_hdr, MY_IP_ADDRESS, dst_ip_address, IPV4_PROTOCOL_ICMP, (uint8_t *)icmp_hdr, icmp_len);
  size_t eth_len = create_ethernet_frame(packet, MY_MAC_ADDRESS, NULL_MAC_ADDRESS, ETH_TYPE_IPV4, (uint8_t *)ipv4_hdr, ipv4_len);

  return eth_len;
}


void send_icmp_echo_request() {
  struct arp_entry *entry = arp_lookup(dst_ip_address);
  uint8_t packet[128];
  size_t packet_size = prepare_icmp_req(packet, dst_ip_address);

  if (entry == NULL) {
    printf("ARP entry not found, sending ARP request\n");
    send_arp_request(dst_ip_address);

    if (enqueue_deferred_packet(packet, packet_size,
        ((uint32_t)dst_ip_address[0] << 24) |
        ((uint32_t)dst_ip_address[1] << 16) |
        ((uint32_t)dst_ip_address[2] << 8) |
        ((uint32_t)dst_ip_address[3])) != 0) {
      printf("Failed to enqueue deferred packet\n");
    }
    return;
  }

  struct ethernet_hdr *eth_hdr = (struct ethernet_hdr *)packet;
  memcpy(eth_hdr->dst_mac, entry->mac, 6);

  virtio_net_transmit(packet, packet_size);
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

void handle_icmp(struct ethernet_hdr *eth_hdr, struct ipv4_hdr *ipv4_hdr, uint32_t len) {
  struct icmp_hdr *icmp_hdr = (struct icmp_hdr *)(ipv4_hdr + 1);
  if (icmp_hdr->type == ICMP_TYPE_ECHO_REQUEST) {
    handle_icmp_echo_request(eth_hdr, ipv4_hdr, icmp_hdr);
  } else if (icmp_hdr->type == ICMP_TYPE_ECHO_REPLY) {
    printf("Received ICMP Echo Reply\n");
  } else {
    printf("Unknown ICMP type %d\n", icmp_hdr->type);
  }
}
