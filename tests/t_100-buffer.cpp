/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "libtests/framework.hpp"
#include <stdexcept>

TEST_CASE("Check buffer usage", "[buffers]") {
	accl::Buffer buffer = accl::Buffer(100);
	const std::string test_string = "hello world";

	// Make sure that the buffer is the correct size
	assert(buffer.getBufferSize() == 100);

	// After appending our test string to the buffer, it should be the size of the test string
	// NK: we use c_str() to guarantee a null-terminated string, and the +1 to include the null termination
	buffer.append(reinterpret_cast<const uint8_t *>(test_string.c_str()), test_string.length() + 1);
	assert(buffer.getDataSize() == test_string.length() + 1);

	// Now lets convert the buffer into a std::string and compare them
	std::string buffer_string(reinterpret_cast<const char *>(buffer.getData()), test_string.length());
	// Next we compare them...
	assert(test_string == buffer_string);

	// The buffer getDataSize() should be 0 after a clear
	buffer.clear();
	assert(buffer.getDataSize() == 0);
}

TEST_CASE("Check we cannot exceed buffer size with append", "[buffers]") {
	accl::Buffer buffer = accl::Buffer(20);

	const std::string test_string = "hello world";

	buffer.append(reinterpret_cast<const uint8_t *>(test_string.c_str()), test_string.length());

	SECTION("Expect a out_of_range is thrown if we try exceed the buffer size") {
		REQUIRE_THROWS_AS(buffer.append(reinterpret_cast<const uint8_t *>(test_string.c_str()), test_string.length()),
						  std::out_of_range);
	}
}

TEST_CASE("Check copying data into the buffer manually and setting size", "[buffers]") {
	accl::Buffer buffer = accl::Buffer(100);
	const std::string test_string = "hello world";

	// Grab pointer to the buffer contents
	const uint8_t *contents = buffer.getData();
	std::memcpy((void *)contents, test_string.c_str(), test_string.length() + 1);

	// Now set the buffer contents size
	buffer.setDataSize(test_string.length() + 1);

	// Now lets convert the buffer into a std::string and compare them
	std::string buffer_string(reinterpret_cast<const char *>(buffer.getData()), test_string.length());
	// Next we compare them...
	assert(test_string == buffer_string);
}

TEST_CASE("Check setting data size manually exceeding the buffer size", "[buffers]") {
	accl::Buffer buffer = accl::Buffer(5);

	SECTION("Expect a out_of_range is thrown if we try exceed the buffer size using setDataSize") {
		REQUIRE_THROWS_AS(buffer.setDataSize(6), std::out_of_range);
	}
}
