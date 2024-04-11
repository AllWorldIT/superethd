/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "libaccl/sequence_data_generator.hpp"
#include "libtests/framework.hpp"

TEST_CASE("Check checksum generation using compute_checksum", "[checksums]") {
	size_t buffer_size{0xffff};

	accl::SequenceDataGenerator seq = accl::SequenceDataGenerator(buffer_size);
	std::vector<uint8_t> test_buffer = seq.asBytes();

	uint16_t checksum = compute_checksum((uint8_t *)test_buffer.data(), buffer_size);

	REQUIRE(checksum == 0xb1e6);
}

TEST_CASE("Check checksum generation using partial checksum", "[checksums]") {
	size_t buffer_size{0xffff};

	accl::SequenceDataGenerator seq = accl::SequenceDataGenerator(buffer_size);
	std::vector<uint8_t> test_buffer = seq.asBytes();

	uint32_t partial_checksum = compute_checksum_partial((uint8_t *)test_buffer.data(), buffer_size, 0);
	uint16_t checksum = compute_checksum_finalize(partial_checksum);

	REQUIRE(checksum == 0xb1e6);
}
