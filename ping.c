#include "virtio.h"
#include "ping.h"

#define MY_IP_ADDRESS      0xC0A80064 // 192.168.0.100
#define DEST_IP_ADDRESS    0xC0A80001 // 192.168.0.1

uint16_t calculate_checksum(void *data, int len) {
  uint32_t sum = 0;
  uint16_t *ptr = (uint16_t *)data;

  for (int i = 0; i < len / 2; i++) {
    sum += ptr[i];
    if (sum > 0xFFFF) {
      sum -= 0xFFFF;
    }
  }

  if (len % 2 == 1) {
    sum += *((uint8_t *)data + len - 1);
    if (sum > 0xFFFF) {
      sum -= 0xFFFF;
    }
  }

  return ~sum;
}


// ICMPパケット送信関数
void send_icmp_echo_request() {
  printf("Sending ICMP Echo Request to 192.168.0.1...\n");

  // パケットバッファ（IPヘッダ + ICMPヘッダ + データ）
  uint8_t packet[64];
  
  struct ipv4_header *ip_hdr = (struct ipv4_header *)packet;
  struct icmp_header *icmp_hdr = (struct icmp_header *)(packet + sizeof(struct ipv4_header));

  // IPヘッダ設定
  ip_hdr->version_ihl = (4 << 4) | (sizeof(struct ipv4_header) / 4); // IPv4, ヘッダ長=20バイト
  ip_hdr->type_of_service = 0;
  ip_hdr->total_length = htons(sizeof(packet)); // 全体の長さ（IP + ICMP）
  ip_hdr->identification = htons(1);
  ip_hdr->flags_fragment_offset = htons(0);
  ip_hdr->ttl = 64;
  ip_hdr->protocol = 1; // ICMPプロトコル番号
  ip_hdr->header_checksum = 0;
  ip_hdr->src_ip = htonl(MY_IP_ADDRESS);
  ip_hdr->dst_ip = htonl(DEST_IP_ADDRESS);

  // IPヘッダチェックサム計算
  ip_hdr->header_checksum = calculate_checksum(ip_hdr, sizeof(struct ipv4_header));

  // ICMPヘッダ設定
  icmp_hdr->type = 8;
  icmp_hdr->code = 0;
  icmp_hdr->checksum = 0;
  icmp_hdr->identifier = htons(12345);
  icmp_hdr->sequence = htons(1);

  // データ部分（任意のペイロード）
  char *data = (char *)(packet + sizeof(struct ipv4_header) + sizeof(struct icmp_header));
  strcpy(data, "PING TEST");

  // ICMPチェックサム計算（ヘッダ + データ部分）
  icmp_hdr->checksum = calculate_checksum(icmp_hdr, sizeof(struct icmp_header) + strlen(data));

  // Virtio-net経由で送信
  virtio_net_transmit(packet, sizeof(packet));

  printf("ICMP Echo Request sent successfully\n");
}
