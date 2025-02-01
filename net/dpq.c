#include "dpq.h"
#include "eth.h"
#include "virtnet.h"
#include "../common.h"

/* Deferred Packet Queue Implmentation */

int deferred_queue_head = 0;
int deferred_queue_tail = 0;

int is_deferred_queue_full() {
  return ((deferred_queue_tail + 1) % DPQ_SIZE) == deferred_queue_head;
}

int is_deferred_queue_empty() {
  return deferred_queue_head == deferred_queue_tail;
}

int enqueue_deferred_packet(uint8_t *packet, size_t packet_size, uint32_t dst_ip_address) {
  if (is_deferred_queue_full()) {
    printf("Deferred queue is full, dropping packet\n");
    return -1;
  }

  struct deferred_packet *entry = &deferred_queue[deferred_queue_tail];
  memcpy(entry->packet, packet, packet_size);
  entry->packet_size = packet_size;
  entry->dst_ip_address = dst_ip_address;

  deferred_queue_tail = (deferred_queue_tail + 1) % DPQ_SIZE; // ring buffer
  return 0;
}

void process_deferred_packets(uint32_t dst_ip_address, uint8_t *mac_address) {
  int current = deferred_queue_head;

  while (current != deferred_queue_tail) {
    struct deferred_packet *entry = &deferred_queue[current];

    if (entry->dst_ip_address == dst_ip_address) {
      struct ethernet_hdr *eth_hdr = (struct ethernet_hdr *)entry->packet;
      memcpy(eth_hdr->dst_mac, mac_address, 6);

      virtio_net_transmit(entry->packet, entry->packet_size);
      printf("Deferred packet sent\n");

      entry->dst_ip_address = 0;
    }

    current = (current + 1) % DPQ_SIZE;
  }

  while (deferred_queue_head != deferred_queue_tail &&
         deferred_queue[deferred_queue_head].dst_ip_address == 0) {
    deferred_queue_head = (deferred_queue_head + 1) % DPQ_SIZE;
  }
}
