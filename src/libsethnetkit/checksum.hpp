/*
 * Checksum functions.
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
#include <stddef.h>
#include <stdint.h>
}

uint32_t compute_checksum_partial(uint8_t *addr8, size_t count, uint32_t sum);
uint16_t compute_checksum_finalize(uint32_t sum);
uint16_t compute_checksum(uint8_t *addr8, int count);
