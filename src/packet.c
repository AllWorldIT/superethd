#include <stdint.h>


// Function to detect sequence number wrapping
int sequence_wrapping(uint32_t cur, uint32_t prev) {
	// Check if the current sequence number is less than the previous one
	// while accounting for wrapping
	return cur < prev && (prev - cur) < (UINT32_MAX / 2);
}

