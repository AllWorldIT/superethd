#ifndef __CHECKSUM_H__
#define __CHECKSUM_H__

#include <stddef.h>
#include <stdint.h>

extern uint32_t compute_checksum_partial(uint8_t *addr8, size_t count, uint32_t sum);
extern uint16_t compute_checksum_finalize(uint32_t sum);
extern uint16_t compute_checksum(uint8_t *addr8, int count);

#endif