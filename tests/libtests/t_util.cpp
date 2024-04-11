/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

extern "C" {
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

uint8_t hex_to_byte(const char *hex) {
	uint8_t byte;
	sscanf(hex, "%2hhx", &byte);
	return byte;
}

uint8_t swap_nibbles(uint8_t byte) { return (byte << 4) | (byte >> 4); }
