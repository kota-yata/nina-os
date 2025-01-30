#pragma once
#include "../common.h"

#define DPQ_SIZE 16

struct deferred_packet {
  uint8_t packet[128];
  size_t packet_size;
  uint32_t dst_ip_address;
  struct deferred_packet *next;
};

static struct deferred_packet deferred_queue[DPQ_SIZE];

int is_deferred_queue_full();
int is_deferred_queue_empty();
int enqueue_deferred_packet(uint8_t *packet, size_t packet_size, uint32_t dst_ip_address);
void process_deferred_packets(uint32_t dst_ip_address, uint8_t *mac_address);

