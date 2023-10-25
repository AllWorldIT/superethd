/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <cstdint>

/*
 * Endian types
 */

// Set up our BE and LE types
using seth_be16_t = uint16_t;
using seth_be32_t = uint32_t;
using seth_be64_t = uint64_t;
using seth_le16_t = uint16_t;
using seth_le32_t = uint32_t;
using seth_le64_t = uint64_t;

// Pull in compiler optimized byte swapping
inline uint16_t seth_bswap16(uint16_t x) { return __builtin_bswap16(x); }
inline uint32_t seth_bswap32(uint32_t x) { return __builtin_bswap32(x); }
inline uint64_t seth_bswap64(uint64_t x) { return __builtin_bswap64(x); }


/*
 * Compile-time endianness detection
 */

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

#if SETH_BYTE_ORDER == SETH_BIG_ENDIAN

inline seth_le16_t seth_cpu_to_le_16(uint16_t x) { return seth_bswap16(x); }
inline seth_le32_t seth_cpu_to_le_32(uint32_t x) { return seth_bswap32(x); }
inline seth_le64_t seth_cpu_to_le_64(uint64_t x) { return seth_bswap64(x); }

inline uint16_t seth_le_to_cpu_16(seth_le16_t x) { return seth_bswap16(x); }
inline uint32_t seth_le_to_cpu_32(seth_le32_t x) { return seth_bswap32(x); }
inline uint64_t seth_le_to_cpu_64(seth_le64_t x) { return seth_bswap64(x); }

inline seth_be16_t seth_cpu_to_be_16(uint16_t x) { return x; }
inline seth_be32_t seth_cpu_to_be_32(uint32_t x) { return x; }
inline seth_be64_t seth_cpu_to_be_64(uint64_t x) { return x; }

inline uint16_t seth_be_to_cpu_16(seth_be16_t x) { return x; }
inline uint32_t seth_be_to_cpu_32(seth_be32_t x) { return x; }
inline uint64_t seth_be_to_cpu_64(seth_be64_t x) { return x; }

#elif SETH_BYTE_ORDER == SETH_LITTLE_ENDIAN

inline seth_le16_t seth_cpu_to_le_16(uint16_t x) { return x; }
inline seth_le32_t seth_cpu_to_le_32(uint32_t x) { return x; }
inline seth_le64_t seth_cpu_to_le_64(uint64_t x) { return x; }

inline uint16_t seth_le_to_cpu_16(seth_le16_t x) { return x; }
inline uint32_t seth_le_to_cpu_32(seth_le32_t x) { return x; }
inline uint64_t seth_le_to_cpu_64(seth_le64_t x) { return x; }

inline seth_be16_t seth_cpu_to_be_16(uint16_t x) { return seth_bswap16(x); }
inline seth_be32_t seth_cpu_to_be_32(uint32_t x) { return seth_bswap32(x); }
inline seth_be64_t seth_cpu_to_be_64(uint64_t x) { return seth_bswap64(x); }

inline uint16_t seth_be_to_cpu_16(seth_be16_t x) { return seth_bswap16(x); }
inline uint32_t seth_be_to_cpu_32(seth_be32_t x) { return seth_bswap32(x); }
inline uint64_t seth_be_to_cpu_64(seth_be64_t x) { return seth_bswap64(x); }

#else
#error Unsupported endianness.
#endif


