#pragma once
#include "eth.h"
#include "ipv4.h"

void send_icmp_echo_request();
void handle_icmp_echo_request(struct ethernet_hdr *req_eth_hdr, struct ipv4_hdr *req_ip_hdr, struct icmp_hdr *req_icmp_hdr);
