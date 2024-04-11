/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "util.hpp"
#include <arpa/inet.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

/**
 * @brief Convert a uint8_t array into a char buffer.
 *
 * @param array uint8_t array to convert.
 * @param length Length of array.
 * @return char* char buffer.
 */
char *uint8_array_to_char_buffer(const uint8_t *array, size_t length) {
	// Allocate memory for the buffer (+1 for the null terminator)
	char *buffer = (char *)malloc(length + 1);
	if (!buffer) {
		return NULL; // Memory allocation failed
	}

	// Copy the values
	for (size_t i = 0; i < length; i++) {
		buffer[i] = (char)array[i];
	}

	// Null-terminate the buffer
	buffer[length] = '\0';

	return buffer;
}

/**
 * @brief Convert a string to an in6_addr.
 *
 * @param str String to convert into an in6_addr.
 * @param result Pointer to the in6_addr to store the result in.
 * @return int Returns 1 on success and 0 on failure.
 */
int to_sin6addr(const char *str, struct in6_addr *result) {
	// Try converting as IPv6 address
	if (inet_pton(AF_INET6, str, result) > 0) {
		return 1; // Successfully converted as IPv6
	}

	// Try converting as IPv4 address and map it to IPv6
	struct in_addr ipv4_addr;
	if (inet_pton(AF_INET, str, &ipv4_addr) > 0) {
		// Convert IPv4 address to IPv6-mapped IPv6 address
		memset(result, 0, sizeof(struct in6_addr));
		result->s6_addr[10] = 0xFF;
		result->s6_addr[11] = 0xFF;
		memcpy(&result->s6_addr[12], &ipv4_addr.s_addr, sizeof(ipv4_addr.s_addr));
		return 1; // Successfully converted as IPv4-mapped IPv6
	}

	// Conversion failed
	return 0;
}

/**
 * @brief Return if the provided in6_addr is an IPv4 mapped IPv6 address.
 *
 * @param ipv6Addr IPv6 address to check.
 * @return int Returns `true` on IPv4 and `false` on IPv6.
 */

inline int is_ipv4_mapped_ipv6(const struct in6_addr *addr) {
	const uint16_t *addrBlocks = (const uint16_t *)(addr->s6_addr);

	// Check the pattern for IPv4-mapped IPv6 addresses
	return (addrBlocks[0] == 0x0000 && addrBlocks[1] == 0x0000 && addrBlocks[2] == 0x0000 && addrBlocks[3] == 0x0000 &&
			addrBlocks[4] == 0x0000 && addrBlocks[5] == 0xffff);
}

/**
 * @brief Get the max payload size from MSS and the destination IP type.
 *
 * @param max_packet_size Maximum packet size.
 * @param dest_addr6 Destination IP address in in6_addr.
 * @return uint16_t Maximum payload size.
 */

uint16_t get_l4mtu(uint16_t max_packet_size, struct in6_addr *dest_addr6) {
	// Set the initial maximum payload size to the specified max packet size
	uint16_t max_payload_size = max_packet_size;

	// Reduce by IP header size
	if (is_ipv4_mapped_ipv6(dest_addr6))
		max_payload_size -= 20; // IPv4 header
	else
		max_payload_size -= 40; // IPv6 header
	max_payload_size -= 8;		// UDP frame

	return max_payload_size;
}

/**
 * @brief Get the max ethernet frame size based on MTU size. This is the raw L2MTU we support.
 *
 * @param mtu Device MTU.
 * @return uint16_t Maximum packet size.
 */

uint16_t get_l2mtu_from_mtu(uint16_t mtu) {
	// Set the initial maximum frame size to the specified MTU
	uint16_t frame_size = mtu;

	// Maximum ethernet frame size is 22 bytes, ethernet header + 802.1ad (8 bytes)
	frame_size += 14 + 8;

	return frame_size;
}

/**
 * @brief Detect if the packet sequence is wrapping.
 *
 * @param cur Current sequence.
 * @param prev Previous sequence.
 * @return int 1 if wrapping, 0 if not.
 */
int is_sequence_wrapping(uint32_t cur, uint32_t prev) {
	// Check if the current sequence number is less than the previous one
	// while accounting for wrapping
	return cur < prev && (prev - cur) > (UINT32_MAX / 2);
}