#ifndef ENDIAN_H
#define ENDIAN_H

#include <stdint.h>

/*
 * Compile-time endianness detection
 */

#define SET_BIG_ENDIAN 1
#define SET_LITTLE_ENDIAN 2
#if defined __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SET_BYTE_ORDER SET_BIG_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SET_BYTE_ORDER SET_LITTLE_ENDIAN
#endif
#else
#if !defined(SET_BYTE_ORDER)
#error Unknown endianness.
#endif
#endif

// Static byte swap operations
#define SET_STATIC_BSWAP16(v) ((((uint16_t)(v)&UINT16_C(0x00ff)) << 8) | (((uint16_t)(v)&UINT16_C(0xff00)) >> 8))

#define SET_STATIC_BSWAP32(v)                                                                     \
	((((uint32_t)(v)&UINT32_C(0x000000ff)) << 24) | (((uint32_t)(v)&UINT32_C(0x0000ff00)) << 8) | \
	 (((uint32_t)(v)&UINT32_C(0x00ff0000)) >> 8) | (((uint32_t)(v)&UINT32_C(0xff000000)) >> 24))

#define SET_STATIC_BSWAP64(v)                                                                                      \
	((((uint64_t)(v)&UINT64_C(0x00000000000000ff)) << 56) | (((uint64_t)(v)&UINT64_C(0x000000000000ff00)) << 40) | \
	 (((uint64_t)(v)&UINT64_C(0x0000000000ff0000)) << 24) | (((uint64_t)(v)&UINT64_C(0x00000000ff000000)) << 8) |  \
	 (((uint64_t)(v)&UINT64_C(0x000000ff00000000)) >> 8) | (((uint64_t)(v)&UINT64_C(0x0000ff0000000000)) >> 24) |  \
	 (((uint64_t)(v)&UINT64_C(0x00ff000000000000)) >> 40) | (((uint64_t)(v)&UINT64_C(0xff00000000000000)) >> 56))

#if SET_BYTE_ORDER == SET_BIG_ENDIAN

#define SET_BE16(v) (set_be16_t)(v)
#define SET_BE32(v) (set_be32_t)(v)
#define SET_BE64(v) (set_be64_t)(v)
#define SET_LE16(v) (set_le16_t)(SET_STATIC_BSWAP16(v))
#define SET_LE32(v) (set_le32_t)(SET_STATIC_BSWAP32(v))
#define SET_LE64(v) (set_le64_t)(SET_STATIC_BSWAP64(v))

#define set_cpu_to_le_16(x) set_bswap16(x)
#define set_cpu_to_le_32(x) set_bswap32(x)
#define set_cpu_to_le_64(x) set_bswap64(x)

#define set_cpu_to_be_16(x) (x)
#define set_cpu_to_be_32(x) (x)
#define set_cpu_to_be_64(x) (x)

#define set_le_to_cpu_16(x) set_bswap16(x)
#define set_le_to_cpu_32(x) set_bswap32(x)
#define set_le_to_cpu_64(x) set_bswap64(x)

#define set_be_to_cpu_16(x) (x)
#define set_be_to_cpu_32(x) (x)
#define set_be_to_cpu_64(x) (x)

#elif SET_BYTE_ORDER == SET_LITTLE_ENDIAN

#define SET_BE16(v) (set_be16_t)(SET_STATIC_BSWAP16(v))
#define SET_BE32(v) (set_be32_t)(SET_STATIC_BSWAP32(v))
#define SET_BE64(v) (set_be64_t)(SET_STATIC_BSWAP64(v))
#define SET_LE16(v) (set_le16_t)(v)
#define SET_LE32(v) (set_le32_t)(v)
#define SET_LE64(v) (set_le64_t)(v)

#define set_cpu_to_le_16(x) (x)
#define set_cpu_to_le_32(x) (x)
#define set_cpu_to_le_64(x) (x)

#define set_cpu_to_be_16(x) set_bswap16(x)
#define set_cpu_to_be_32(x) set_bswap32(x)
#define set_cpu_to_be_64(x) set_bswap64(x)

#define set_le_to_cpu_16(x) (x)
#define set_le_to_cpu_32(x) (x)
#define set_le_to_cpu_64(x) (x)

#define set_be_to_cpu_16(x) set_bswap16(x)
#define set_be_to_cpu_32(x) set_bswap32(x)
#define set_be_to_cpu_64(x) set_bswap64(x)

#else
#error Unsupported endianness.
#endif

typedef uint16_t set_be16_t;
typedef uint32_t set_be32_t;
typedef uint64_t set_be64_t;
typedef uint16_t set_le16_t;
typedef uint32_t set_le32_t;
typedef uint64_t set_le64_t;

// Pull in compiler optimized byte swapping
#define set_bswap16(x) __builtin_bswap16(x)
#define set_bswap32(x) __builtin_bswap32(x)
#define set_bswap64(x) __builtin_bswap64(x)

#define __set_packed __attribute__((packed))

#endif