/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <cstddef>
#include <cstdint>

uint32_t compute_checksum_partial(uint8_t *addr8, size_t count, uint32_t sum);
uint16_t compute_checksum_finalize(uint32_t sum);
uint16_t compute_checksum(uint8_t *addr8, int count);
