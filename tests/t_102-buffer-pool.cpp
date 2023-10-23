/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "libtests/framework.hpp"

TEST_CASE("Check creating a buffer pool works", "[buffers]") {
	accl::BufferPool buffer_pool = accl::BufferPool(1024, 10);

	assert(buffer_pool.getBufferCount() == 10);
}

TEST_CASE("Check popping a buffer from the pool works", "[buffers]") {
	accl::BufferPool buffer_pool = accl::BufferPool(1024, 10);

	assert(buffer_pool.getBufferCount() == 10);

	auto buffer = buffer_pool.pop();

	assert(buffer->getBufferSize() == 1024);
	assert(buffer->getDataSize() == 0);
}

TEST_CASE("Check that we get an exception when we pop too many buffers from the pool", "[buffers]") {
	accl::BufferPool buffer_pool = accl::BufferPool(1024, 1);

	auto buffer = buffer_pool.pop();

	SECTION("Expect a bad_alloc is thrown if we try exceed the buffer size") {
		REQUIRE_THROWS_AS(buffer_pool.pop(), std::bad_alloc);
	}
}

TEST_CASE("Check that we can push a buffer back into the pool", "[buffers]") {
	accl::BufferPool buffer_pool = accl::BufferPool(1024, 1);

	auto buffer = buffer_pool.pop();

	// Check the buffer pool is empty
	assert(buffer_pool.getBufferCount() == 0);

	buffer_pool.push(buffer);

	// Check the buffer was added back
	assert(buffer_pool.getBufferCount() == 1);
}

TEST_CASE("Check that we can add our own buffer to the pool", "[buffers]") {
	accl::BufferPool buffer_pool = accl::BufferPool(1024, 0);

	accl::Buffer buffer(1024);

	std::unique_ptr<accl::Buffer> buffer_ptr = std::make_unique<accl::Buffer>(std::move(buffer));

	buffer_pool.push(buffer_ptr);

	// Check the buffer was added back
	assert(buffer_pool.getBufferCount() == 1);
}

TEST_CASE("Check that we get an exception adding an incorrectly sized buffer to the pool", "[buffers]") {
	accl::BufferPool buffer_pool = accl::BufferPool(1024, 1);

	accl::Buffer buffer(100);

	std::unique_ptr<accl::Buffer> buffer_ptr = std::make_unique<accl::Buffer>(std::move(buffer));

	SECTION("Expect a invalid_argument is thrown if we try add a buffer of incorrect size") {
		REQUIRE_THROWS_AS(buffer_pool.push(buffer_ptr), std::invalid_argument);
	}

	// Check the buffer was added back
	assert(buffer_pool.getBufferCount() == 1);
}

TEST_CASE("Check that we get an exception adding an incorrectly sized buffers to the pool", "[buffers]") {
	accl::BufferPool buffer_pool = accl::BufferPool(1024, 1);

	std::vector<std::unique_ptr<accl::Buffer>> buffers;

	auto buffer_ptr = std::make_unique<accl::Buffer>(1000);
	buffers.push_back(std::move(buffer_ptr));

	SECTION("Expect a invalid_argument is thrown if we try add buffers of incorrect size") {
		REQUIRE_THROWS_AS(buffer_pool.push(buffers), std::invalid_argument);
	}

	// Check the buffer was added back
	assert(buffer_pool.getBufferCount() == 1);
}

TEST_CASE("Check we get all the buffers when we popAll()", "[buffers]") {
	accl::BufferPool buffer_pool = accl::BufferPool(1024, 5);

	auto buffers = buffer_pool.popAll();

	// Check we got 5 buffers
	assert(buffers.size() == 5);

	// Also check that buffer_pool is now empty
	assert(buffer_pool.getBufferCount() == 0);
}

TEST_CASE("Check we can push multiple buffers back into the pool", "[buffers]") {
	accl::BufferPool buffer_pool = accl::BufferPool(1024, 5);

	auto buffers = buffer_pool.popAll();

	// Check we got 5 buffers
	assert(buffers.size() == 5);

	// Add buffers back
	buffer_pool.push(buffers);

	// Check buffers is empty
	assert(buffers.size() == 0);

	// Check the buffers were added back
	assert(buffer_pool.getBufferCount() == 5);
}