/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <cstdint>

namespace accl {

/*
 * Endian types
 */

// Set up our BE and LE types
using be16_t = uint16_t;
using be32_t = uint32_t;
using be64_t = uint64_t;
using le16_t = uint16_t;
using le32_t = uint32_t;
using le64_t = uint64_t;

// Pull in compiler optimized byte swapping
inline uint16_t bswap16(uint16_t x) { return __builtin_bswap16(x); }
inline uint32_t bswap32(uint32_t x) { return __builtin_bswap32(x); }
inline uint64_t bswap64(uint64_t x) { return __builtin_bswap64(x); }

/*
 * Compile-time endianness detection
 */

#define ACCL_BIG_ENDIAN 1
#define ACCL_LITTLE_ENDIAN 2
#if defined __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ACCL_BYTE_ORDER ACCL_BIG_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ACCL_BYTE_ORDER ACCL_LITTLE_ENDIAN
#endif
#else
#if !defined(ACCL_BYTE_ORDER)
#error Unknown endianness.
#endif
#endif

#if ACCL_BYTE_ORDER == ACCL_BIG_ENDIAN

inline le16_t cpu_to_le_16(uint16_t x) { return bswap16(x); }
inline le32_t cpu_to_le_32(uint32_t x) { return bswap32(x); }
inline le64_t cpu_to_le_64(uint64_t x) { return bswap64(x); }

inline uint16_t le_to_cpu_16(le16_t x) { return bswap16(x); }
inline uint32_t le_to_cpu_32(le32_t x) { return bswap32(x); }
inline uint64_t le_to_cpu_64(le64_t x) { return bswap64(x); }

inline be16_t cpu_to_be_16(uint16_t x) { return x; }
inline be32_t cpu_to_be_32(uint32_t x) { return x; }
inline be64_t cpu_to_be_64(uint64_t x) { return x; }

inline uint16_t be_to_cpu_16(be16_t x) { return x; }
inline uint32_t be_to_cpu_32(be32_t x) { return x; }
inline uint64_t be_to_cpu_64(be64_t x) { return x; }

#elif ACCL_BYTE_ORDER == ACCL_LITTLE_ENDIAN

inline le16_t cpu_to_le_16(uint16_t x) { return x; }
inline le32_t cpu_to_le_32(uint32_t x) { return x; }
inline le64_t cpu_to_le_64(uint64_t x) { return x; }

inline uint16_t le_to_cpu_16(le16_t x) { return x; }
inline uint32_t le_to_cpu_32(le32_t x) { return x; }
inline uint64_t le_to_cpu_64(le64_t x) { return x; }

inline be16_t cpu_to_be_16(uint16_t x) { return bswap16(x); }
inline be32_t cpu_to_be_32(uint32_t x) { return bswap32(x); }
inline be64_t cpu_to_be_64(uint64_t x) { return bswap64(x); }

inline uint16_t be_to_cpu_16(be16_t x) { return bswap16(x); }
inline uint32_t be_to_cpu_32(be32_t x) { return bswap32(x); }
inline uint64_t be_to_cpu_64(be64_t x) { return bswap64(x); }

#else
#error Unsupported endianness.
#endif

} // namespace accl