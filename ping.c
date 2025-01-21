#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/types.h>

// Compute checksum for IP/ICMP headers
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int main() {
    int sockfd;
    char buffer[65536];
    struct sockaddr_in addr;

    // Create raw socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        socklen_t addr_len = sizeof(addr);
        ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                             (struct sockaddr *)&addr, &addr_len);
        if (n < 0) {
            perror("Failed to receive packet");
            continue;
        }

        struct iphdr *ip_header = (struct iphdr *)buffer;
        struct icmphdr *icmp_header =
            (struct icmphdr *)(buffer + (ip_header->ihl * 4));

        // Check if it's an ICMP echo request
        if (icmp_header->type == ICMP_ECHO) {
            printf("Received ICMP Echo Request\n");

            // Modify ICMP header for Echo Reply
            icmp_header->type = ICMP_ECHOREPLY;
            icmp_header->checksum = 0;
            icmp_header->checksum =
                checksum(icmp_header, n - (ip_header->ihl * 4));

            // Swap source and destination IP addresses
            struct in_addr temp = *(struct in_addr *)&ip_header->saddr;
            ip_header->saddr = ip_header->daddr;
            ip_header->daddr = *(uint32_t *)&temp;

            // Send reply
            if (sendto(sockfd, buffer, n, 0, (struct sockaddr *)&addr,
                       addr_len) < 0) {
                perror("Failed to send packet");
            } else {
                printf("Sent ICMP Echo Reply\n");
            }
        }
    }

    close(sockfd);
    return 0;
}
