/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <format>

// Debug output macro
#ifdef DEBUG
//#define DEBUG_PRINT(fmt, ...) fprintf(stderr, "%s(%s:%i): " fmt "\n", __func__, __FILE__, __LINE__, ##__VA_ARGS__)
#define DEBUG_CERR(fmt, ...) std::cerr << std::format("{}({}:{}): " fmt, __func__, __FILE__, __LINE__, ##__VA_ARGS__) << std::endl;
#else
#define DEBUG_PRINT(fmt, ...) ((void)0)
#endif

#ifdef UNIT_TESTING
#define UT_ASSERT(...) assert(__VA_ARGS__)
#else
#define UT_ASSERT(...) ((void)0)
#endif

// Normal fprintf macro
//#define FPRINTF(fmt, ...) fprintf(stderr, "%s(): " fmt "\n", __func__, ##__VA_ARGS__)

#define CERR(fmt, ...) std::cerr << std::format(fmt, ##__VA_ARGS__) << std::endl;

// Print buffers in various formats
void print_hex_dump(const uint8_t *buffer, size_t length);
void print_in_bits(const uint8_t *buffer, size_t length);
