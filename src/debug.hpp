/*
 * Debug functionality.
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
