/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <cstddef>
#include <cstdint>

// Make the packed attribute easier to use
#define SETH_PACKED_ATTRIBUTES __attribute__((packed))

// Default tunnel name
constexpr char SETH_DEFAULT_TUNNEL_NAME[] = "seth0";

// Default buffer count
inline constexpr size_t SETH_BUFFER_COUNT = 5000;

// Number of messages to get at maximum from recvmm
inline constexpr uint32_t SETH_MAX_RECVMM_MESSAGES = 1000;

// Minimum transmission packet size
inline constexpr uint16_t SETH_MIN_TXSIZE = 1200;

// Maximum transmission packet size
inline constexpr uint16_t SETH_MAX_TXSIZE = 9000;

// Minimum device MTU size
inline constexpr uint16_t SETH_MIN_MTU_SIZE = 1200;

// Maximum device MTU size
inline constexpr uint16_t SETH_MAX_MTU_SIZE = 9198;

/*
 * Other useful constants
 */

inline constexpr uint16_t SETH_MIN_ETHERNET_FRAME_SIZE = 64;
