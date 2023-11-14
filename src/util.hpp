/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <netinet/in.h>

int read_hex_dump_into_buffer(const char *hex_dump, uint8_t **buffer, size_t *length);
char *uint8_array_to_char_buffer(const uint8_t *array, size_t length);

char *create_sequence_data(size_t length);

int to_sin6addr(const char *address_str, struct in6_addr *result);
int is_ipv4_mapped_ipv6(const struct in6_addr *addr);

uint16_t get_l4mtu(uint16_t max_packet_size, struct in6_addr *dest_addr6);
uint16_t get_l2mtu_from_mtu(uint16_t mtu);

int is_sequence_wrapping(uint32_t cur, uint32_t prev);
