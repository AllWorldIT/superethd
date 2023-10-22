/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

extern "C" {
#include <stddef.h>
#include <stdint.h>
}

#include "../endian.hpp"

uint32_t compute_checksum_partial(uint8_t *addr8, size_t count, uint32_t sum) {
	uint16_t *addr = (uint16_t *)addr8;

	while (count > 1) {
		sum += seth_be_to_cpu_16(*addr++);
		count -= 2;
	}

	if (count > 0) {
		sum += *(uint8_t *)addr;
	}

	return sum;
}

uint16_t compute_checksum_finalize(uint32_t sum) {
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}

	return (uint16_t)(~sum);
}

uint16_t compute_checksum(uint8_t *addr8, int count) {
	uint32_t sum = compute_checksum_partial(addr8, count, 0);
	return compute_checksum_finalize(sum);
}

