/*
 * Checksum testing.
 * Copyright (C) 2023, AllWorldIT.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
