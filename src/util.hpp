/*
 * Utility functions.
 * Copyright (C) 2023, AllWorldIT.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

extern "C" {
#include <netinet/in.h>
}

int read_hex_dump_into_buffer(const char *hex_dump, uint8_t **buffer, size_t *length);
char *uint8_array_to_char_buffer(const uint8_t *array, size_t length);

char *create_sequence_data(size_t length);

int to_sin6addr(const char *address_str, struct in6_addr *result);
int is_ipv4_mapped_ipv6(const struct in6_addr *addr);

uint16_t get_max_payload_size(uint8_t max_packet_size, struct in6_addr *dest_addr6);
uint16_t get_max_ethernet_frame_size(uint8_t mtu);

int is_sequence_wrapping(uint32_t cur, uint32_t prev);
