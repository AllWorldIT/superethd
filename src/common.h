
#ifndef COMMON_H
#define COMMON_H

// Debug output macro
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) ((void)0)
#endif

#define STAP_BUFFER_COUNT 5000
#define STAP_MAX_PACKET_SIZE 9216 + sizeof(PacketHeader)
#define STAP_MAX_RECVMM_MESSAGES 1000

#define STAP_MIN_MSS_SIZE 1200
#define STAP_MAX_MSS_SIZE 9000

#define STAP_MIN_MTU_SIZE 1400	// FIXME: Change back to 1500
#define STAP_MAX_MTU_SIZE 9000

#endif