/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

/*
 * Compile-time endianness detection
 */

#include <cstdint>
#define SETH_BIG_ENDIAN 1
#define SETH_LITTLE_ENDIAN 2
#if defined __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SETH_BYTE_ORDER SETH_BIG_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SETH_BYTE_ORDER SETH_LITTLE_ENDIAN
#endif
#else
#if !defined(SETH_BYTE_ORDER)
#error Unknown endianness.
#endif
#endif

// Static byte swap operations
#define SETH_STATIC_BSWAP16(v) ((((uint16_t)(v)&UINT16_C(0x00ff)) << 8) | (((uint16_t)(v)&UINT16_C(0xff00)) >> 8))

#define SETH_STATIC_BSWAP32(v)                                                                                                     \
	((((uint32_t)(v)&UINT32_C(0x000000ff)) << 24) | (((uint32_t)(v)&UINT32_C(0x0000ff00)) << 8) |                                  \
	 (((uint32_t)(v)&UINT32_C(0x00ff0000)) >> 8) | (((uint32_t)(v)&UINT32_C(0xff000000)) >> 24))

#define SETH_STATIC_BSWAP64(v)                                                                                                     \
	((((uint64_t)(v)&UINT64_C(0x00000000000000ff)) << 56) | (((uint64_t)(v)&UINT64_C(0x000000000000ff00)) << 40) |                 \
	 (((uint64_t)(v)&UINT64_C(0x0000000000ff0000)) << 24) | (((uint64_t)(v)&UINT64_C(0x00000000ff000000)) << 8) |                  \
	 (((uint64_t)(v)&UINT64_C(0x000000ff00000000)) >> 8) | (((uint64_t)(v)&UINT64_C(0x0000ff0000000000)) >> 24) |                  \
	 (((uint64_t)(v)&UINT64_C(0x00ff000000000000)) >> 40) | (((uint64_t)(v)&UINT64_C(0xff00000000000000)) >> 56))

#if SETH_BYTE_ORDER == SETH_BIG_ENDIAN

#define SETH_BE16(v) (seth_be16_t)(v)
#define SETH_BE32(v) (seth_be32_t)(v)
#define SETH_BE64(v) (seth_be64_t)(v)
#define SETH_LE16(v) (seth_le16_t)(SETH_STATIC_BSWAP16(v))
#define SETH_LE32(v) (seth_le32_t)(SETH_STATIC_BSWAP32(v))
#define SETH_LE64(v) (seth_le64_t)(SETH_STATIC_BSWAP64(v))

#define seth_cpu_to_le_16(x) seth_bswap16(x)
#define seth_cpu_to_le_32(x) seth_bswap32(x)
#define seth_cpu_to_le_64(x) seth_bswap64(x)

#define seth_cpu_to_be_16(x) (x)
#define seth_cpu_to_be_32(x) (x)
#define seth_cpu_to_be_64(x) (x)

#define seth_le_to_cpu_16(x) seth_bswap16(x)
#define seth_le_to_cpu_32(x) seth_bswap32(x)
#define seth_le_to_cpu_64(x) seth_bswap64(x)

#define seth_be_to_cpu_16(x) (x)
#define seth_be_to_cpu_32(x) (x)
#define seth_be_to_cpu_64(x) (x)

#elif SETH_BYTE_ORDER == SETH_LITTLE_ENDIAN

#define SETH_BE16(v) (seth_be16_t)(SETH_STATIC_BSWAP16(v))
#define SETH_BE32(v) (seth_be32_t)(SETH_STATIC_BSWAP32(v))
#define SETH_BE64(v) (seth_be64_t)(SETH_STATIC_BSWAP64(v))
#define SETH_LE16(v) (seth_le16_t)(v)
#define SETH_LE32(v) (seth_le32_t)(v)
#define SETH_LE64(v) (seth_le64_t)(v)

#define seth_cpu_to_le_16(x) (x)
#define seth_cpu_to_le_32(x) (x)
#define seth_cpu_to_le_64(x) (x)

#define seth_cpu_to_be_16(x) seth_bswap16(x)
#define seth_cpu_to_be_32(x) seth_bswap32(x)
#define seth_cpu_to_be_64(x) seth_bswap64(x)

#define seth_le_to_cpu_16(x) (x)
#define seth_le_to_cpu_32(x) (x)
#define seth_le_to_cpu_64(x) (x)

#define seth_be_to_cpu_16(x) seth_bswap16(x)
#define seth_be_to_cpu_32(x) seth_bswap32(x)
#define seth_be_to_cpu_64(x) seth_bswap64(x)

#else
#error Unsupported endianness.
#endif

// Set up our BE and LE types
using seth_be16_t = uint16_t;
using seth_be32_t = uint32_t;
using seth_be64_t = uint64_t;
using seth_le16_t = uint16_t;
using seth_le32_t = uint32_t;
using seth_le64_t = uint64_t;

// Pull in compiler optimized byte swapping
#define seth_bswap16(x) __builtin_bswap16(x)
#define seth_bswap32(x) __builtin_bswap32(x)
#define seth_bswap64(x) __builtin_bswap64(x)
