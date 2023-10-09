
#ifndef COMMON_H
#define COMMON_H

#define SET_VERSION 1

// Default buffer count
#define SET_BUFFER_COUNT 5000
// Number of messages to get at maximum from recvmm
#define SET_MAX_RECVMM_MESSAGES 1000

// Minimum transmission packet size
#define SET_MIN_TXSIZE 1200
// Maximum transmission packet size
#define SET_MAX_TXSIZE 9000

// Minimum device MTU size
#define SET_MIN_MTU_SIZE 1200
// Maximum device MTU size
#define SET_MAX_MTU_SIZE 9198


/*
 * Other useful constants
 */

#define SET_MIN_ETHERNET_FRAME_SIZE 64


#endif