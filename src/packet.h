#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

/*
 * Compile-time endianness detection
 */

#define STAP_BIG_ENDIAN 1
#define STAP_LITTLE_ENDIAN 2
#if defined __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define STAP_BYTE_ORDER STAP_BIG_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define STAP_BYTE_ORDER STAP_LITTLE_ENDIAN
#endif
#else
#if !defined(STAP_BYTE_ORDER)
#error Unknown endianness.
#endif
#endif

// Static byte swap operations
#define STAP_STATIC_BSWAP16(v) ((((uint16_t)(v)&UINT16_C(0x00ff)) << 8) | (((uint16_t)(v)&UINT16_C(0xff00)) >> 8))

#define STAP_STATIC_BSWAP32(v)                                                                    \
	((((uint32_t)(v)&UINT32_C(0x000000ff)) << 24) | (((uint32_t)(v)&UINT32_C(0x0000ff00)) << 8) | \
	 (((uint32_t)(v)&UINT32_C(0x00ff0000)) >> 8) | (((uint32_t)(v)&UINT32_C(0xff000000)) >> 24))

#define STAP_STATIC_BSWAP64(v)                                                                                     \
	((((uint64_t)(v)&UINT64_C(0x00000000000000ff)) << 56) | (((uint64_t)(v)&UINT64_C(0x000000000000ff00)) << 40) | \
	 (((uint64_t)(v)&UINT64_C(0x0000000000ff0000)) << 24) | (((uint64_t)(v)&UINT64_C(0x00000000ff000000)) << 8) |  \
	 (((uint64_t)(v)&UINT64_C(0x000000ff00000000)) >> 8) | (((uint64_t)(v)&UINT64_C(0x0000ff0000000000)) >> 24) |  \
	 (((uint64_t)(v)&UINT64_C(0x00ff000000000000)) >> 40) | (((uint64_t)(v)&UINT64_C(0xff00000000000000)) >> 56))

#if STAP_BYTE_ORDER == STAP_BIG_ENDIAN

#define STAP_BE16(v) (stap_be16_t)(v)
#define STAP_BE32(v) (stap_be32_t)(v)
#define STAP_BE64(v) (stap_be64_t)(v)
#define STAP_LE16(v) (stap_le16_t)(STAP_STATIC_BSWAP16(v))
#define STAP_LE32(v) (stap_le32_t)(STAP_STATIC_BSWAP32(v))
#define STAP_LE64(v) (stap_le64_t)(STAP_STATIC_BSWAP64(v))

#define stap_cpu_to_le_16(x) stap_bswap16(x)
#define stap_cpu_to_le_32(x) stap_bswap32(x)
#define stap_cpu_to_le_64(x) stap_bswap64(x)

#define stap_cpu_to_be_16(x) (x)
#define stap_cpu_to_be_32(x) (x)
#define stap_cpu_to_be_64(x) (x)

#define stap_le_to_cpu_16(x) stap_bswap16(x)
#define stap_le_to_cpu_32(x) stap_bswap32(x)
#define stap_le_to_cpu_64(x) stap_bswap64(x)

#define stap_be_to_cpu_16(x) (x)
#define stap_be_to_cpu_32(x) (x)
#define stap_be_to_cpu_64(x) (x)

#elif STAP_BYTE_ORDER == STAP_LITTLE_ENDIAN

#define STAP_BE16(v) (stap_be16_t)(STAP_STATIC_BSWAP16(v))
#define STAP_BE32(v) (stap_be32_t)(STAP_STATIC_BSWAP32(v))
#define STAP_BE64(v) (stap_be64_t)(STAP_STATIC_BSWAP64(v))
#define STAP_LE16(v) (stap_le16_t)(v)
#define STAP_LE32(v) (stap_le32_t)(v)
#define STAP_LE64(v) (stap_le64_t)(v)

#define stap_cpu_to_le_16(x) (x)
#define stap_cpu_to_le_32(x) (x)
#define stap_cpu_to_le_64(x) (x)

#define stap_cpu_to_be_16(x) stap_bswap16(x)
#define stap_cpu_to_be_32(x) stap_bswap32(x)
#define stap_cpu_to_be_64(x) stap_bswap64(x)

#define stap_le_to_cpu_16(x) (x)
#define stap_le_to_cpu_32(x) (x)
#define stap_le_to_cpu_64(x) (x)

#define stap_be_to_cpu_16(x) stap_bswap16(x)
#define stap_be_to_cpu_32(x) stap_bswap32(x)
#define stap_be_to_cpu_64(x) stap_bswap64(x)

#else
#error Unsupported endianness.
#endif

typedef uint16_t stap_be16_t;
typedef uint32_t stap_be32_t;
typedef uint64_t stap_be64_t;
typedef uint16_t stap_le16_t;
typedef uint32_t stap_le32_t;
typedef uint64_t stap_le64_t;

// Pull in compiler optimized byte swapping
#define stap_bswap16(x) __builtin_bswap16(x)
#define stap_bswap32(x) __builtin_bswap32(x)
#define stap_bswap64(x) __builtin_bswap64(x)

#define __stap_packed __attribute__((packed))

/*

   Packet Header:
	  |               |               |               |               |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |  Ver  |Opt Len|O|C|    Rsvd.  | Packet Format |    Channel    |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |                            Sequence                           |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |                                                               |
	  ~                    Variable-Length Options                    ~
	  |                                                               |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	- Version: 4 bits.
	- Opt Len: 4 bits, number of 32 bit wide variable options.
	- O: 1 bit, indicates a high priority management packet.
	- C: 1 bit, packet contains critical options that MUST be parsed.
	- Packet Format: 8 bits, this indicates the packet payload format.
	- Channel: 8 bits, Network channel to use.
*/

#define STAP_PACKET_VERSION_V1 0x1

#define STAP_PACKET_FORMAT_ENCAP 0x1

#define STAP_PACKET_HEADER_SIZE sizeof(PacketHeader)

typedef struct __stap_packed {
#if STAP_BYTE_ORDER == STAP_BIG_ENDIAN
	uint8_t ver : 4;
	uint8_t opt_len : 4;

	uint8_t oam : 1;
	uint8_t critical : 1;
	uint8_t reserved : 6;
#else
	uint8_t opt_len : 4;
	uint8_t ver : 4;

	uint8_t reserved : 6;
	uint8_t critical : 1;
	uint8_t oam : 1;

#endif
	uint8_t format;
	uint8_t channel;
	stap_be32_t sequence;
	uint32_t opts[];
} PacketHeader;

// Our own functions
extern int sequence_wrapping(uint32_t cur, uint32_t prev);

#endif
