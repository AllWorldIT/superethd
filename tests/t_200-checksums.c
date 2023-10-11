#include "libtests/framework.h"
#include "checksum.h"
#include "util.h"


Test(checksums, checksum_correctness) {
	size_t buffer_size = 0xffff;

	char *test_buffer = create_sequence_data(0xffff);

	uint32_t partial_checksum = compute_checksum_partial((uint8_t *)test_buffer, buffer_size, 0);
	uint16_t checksum = compute_checksum_finalize(partial_checksum);

	cr_assert(eq(u16, checksum, 0xb1e6), "Ensure checksum matches known result.");
}
