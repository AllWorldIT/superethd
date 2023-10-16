#include "libtests/framework.hpp"


TEST_CASE("Checking checksums generated are correct", "[checksums]") {
	size_t buffer_size = 0xffff;

	char *test_buffer = create_sequence_data(0xffff);

	uint32_t partial_checksum = compute_checksum_partial((uint8_t *)test_buffer, buffer_size, 0);
	uint16_t checksum = compute_checksum_finalize(partial_checksum);

	REQUIRE(checksum == 0xb1e6);
}
