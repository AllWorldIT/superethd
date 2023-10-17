
#pragma once

#include <cstddef>

extern "C" {
#include <stdint.h>
}


// Debug output macro
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) fprintf(stderr, "%s(%s:%i): " fmt "\n", __func__, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) ((void)0)
#endif

#ifdef UNIT_TESTING
#define UT_ASSERT(...) assert(__VA_ARGS__)
#else
#define UT_ASSERT(...) ((void)0)
#endif


// Normal fprintf macro
#define FPRINTF(fmt, ...) fprintf(stderr, "%s(): " fmt "\n", __func__, ##__VA_ARGS__)

#define T_PRINTF(fmt, ...) fprintf(stderr, "%s(%s:%i): " fmt "\n", __func__, __FILE__, __LINE__, ##__VA_ARGS__)


// Print buffers in various formats
void print_hex_dump(const uint8_t *buffer, size_t length);
void print_in_bits(const uint8_t *buffer, size_t length);

// FIXME
// // Packet functions
// void print_ethernet_frame(const ethernet_header_t *packet, uint16_t size);
// void print_ip_header(const ethernet_header_t *packet, uint16_t size);
// void print_ipv4_header(const ethernet_header_t *pkt, uint16_t pkt_size);
// void print_udp_header(const ethernet_header_t *pkt, uint16_t pkt_size);
// void print_icmp_header(const ethernet_header_t *packet, uint16_t size);
