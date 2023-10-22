/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

extern "C" {
#include <stddef.h>
#include <stdint.h>
}

extern uint8_t hex_to_byte(const char *hex);

extern uint8_t swap_nibbles(uint8_t byte);
