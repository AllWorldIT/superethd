#ifndef UTIL_H
#define UTIL_H

#include <netinet/in.h>

extern int read_hex_dump_into_buffer(const char *hex_dump, uint8_t **buffer, size_t *length);
extern char *uint8_array_to_char_buffer(const uint8_t *array, size_t length);

extern char *create_sequence_data(size_t length);

extern int to_sin6addr(const char *address_str, struct in6_addr *result);
extern int is_ipv4_mapped_ipv6(const struct in6_addr *addr);

extern uint16_t get_max_payload_size(uint8_t max_packet_size, struct in6_addr *dest_addr6);
extern uint16_t get_max_ethernet_frame_size(uint8_t mtu);

extern int is_sequence_wrapping(uint32_t cur, uint32_t prev);

#endif