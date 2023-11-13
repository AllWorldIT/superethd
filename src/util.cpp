/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "util.hpp"

#include <arpa/inet.h>

#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_hex_dump_into_buffer(const char *hex_dump, uint8_t **buffer, size_t *length) {
	*buffer = (uint8_t *)malloc(strlen(hex_dump)); // Maximum possible size
	if (!*buffer) {
		return -1; // Memory allocation failed
	}

	size_t offset = 0;
	size_t byte_count = 0;
	const char *cursor = hex_dump;

	while (sscanf(cursor, "%4zx", &offset) == 1) {
		cursor += 5; // Move past offset and space

		for (int i = 0; i < 16 && *cursor && *cursor != '\n'; i++) {
			if (sscanf(cursor, " %2hhx", &(*buffer)[byte_count]) != 1) {
				break; // End of line or invalid data
			}
			byte_count++;
			cursor += 3; // Move past hex byte and space
		}

		while (*cursor && *cursor != '\n') {
			cursor++; // Move to next line
		}
		if (*cursor == '\n') {
			cursor++; // Move past newline
		}
	}

	*buffer = (uint8_t *)realloc(*buffer, byte_count); // Resize to actual size
	*length = byte_count;

	return 0; // Success
}

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

char *create_sequence_data(size_t length) {
	if (length < 2)
		return NULL; // Ensure at least space for one character and a null terminator

	char *buffer = (char *)malloc(length + 1);
	if (!buffer)
		return NULL; // Memory allocation failed

	char letter = 'A';
	char number = '0';
	uint16_t index = 0;

	while (index < length) { // Leave space for null terminator
		buffer[index++] = letter;
		for (number = '0'; number <= '9' && index < length; number++) {
			buffer[index++] = number;
		}

		// Update for the next letter
		if (letter == 'Z') {
			letter = 'A';
		} else {
			letter++;
		}
	}

	buffer[index] = '\0'; // Null terminate the buffer
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

uint16_t get_max_payload_size(uint8_t max_packet_size, struct in6_addr *dest_addr6) {
	// Set the initial maximum payload size to the specified max packet size
	uint8_t max_payload_size = max_packet_size;

	// Reduce by IP header size
	if (is_ipv4_mapped_ipv6(dest_addr6))
		max_payload_size -= 20; // IPv4 header
	else
		max_payload_size -= 40; // IPv6 header
	max_payload_size -= 8;		// UDP frame

	return max_payload_size;
}

/**
 * @brief Get the max ethernet frame size based on MTU size.
 *
 * @param mtu Maximum .
 * @return uint16_t Maximum packet size.
 */

uint16_t get_max_ethernet_frame_size(uint8_t mtu) {
	// Set the initial maximum frame size to the specified MTU
	uint8_t frame_size = mtu;

	// Maximum ethernet frame size is 22 bytes, ethernet header + 802.1ad (8 bytes)
	frame_size -= 14 + 8;

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
	return cur < prev && (prev - cur) < (UINT32_MAX / 2);
}