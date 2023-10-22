/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "libtests/framework.hpp"


TEST_CASE("Checking checksums generated are correct", "[checksums]") {
	size_t buffer_size = 0xffff;


	// TODO: convert to c++

	char *test_buffer = create_sequence_data(0xffff);

	uint32_t partial_checksum = compute_checksum_partial((uint8_t *)test_buffer, buffer_size, 0);
	uint16_t checksum = compute_checksum_finalize(partial_checksum);

	REQUIRE(checksum == 0xb1e6);

	free(test_buffer);
}
