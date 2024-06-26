/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "libtests/framework.hpp"

TEST_CASE("Check creating a buffer pool works", "[buffers]") {
	accl::BufferPool<PacketBuffer> buffer_pool(1024, 10);

	REQUIRE(buffer_pool.getBufferCount() == 10);
}

TEST_CASE("Check popping a buffer from the pool works", "[buffers]") {
	accl::BufferPool<PacketBuffer> buffer_pool(1024, 10);

	REQUIRE(buffer_pool.getBufferCount() == 10);

	auto buffer = buffer_pool.pop();

	REQUIRE(buffer->getBufferSize() == 1024);
	REQUIRE(buffer->getDataSize() == 0);
}

TEST_CASE("Check that we get an exception when we pop too many buffers from the pool", "[buffers]") {
	accl::BufferPool<PacketBuffer> buffer_pool = accl::BufferPool<PacketBuffer>(1024, 1);

	auto buffer = buffer_pool.pop();

	SECTION("Expect a bad_alloc is thrown if we try exceed the buffer size") {
		REQUIRE_THROWS_AS(buffer_pool.pop(), std::bad_alloc);
	}
}

TEST_CASE("Check that we can push a buffer back into the pool", "[buffers]") {
	accl::BufferPool<PacketBuffer> buffer_pool = accl::BufferPool<PacketBuffer>(1024, 1);

	auto buffer = buffer_pool.pop();

	// Check the buffer pool is empty
	REQUIRE(buffer_pool.getBufferCount() == 0);

	buffer_pool.push(std::move(buffer));

	// Check the buffer was added back
	REQUIRE(buffer_pool.getBufferCount() == 1);
}

TEST_CASE("Check that we can add our own buffer to the pool", "[buffers]") {
	accl::BufferPool<PacketBuffer> buffer_pool = accl::BufferPool<PacketBuffer>(1024, 0);

	PacketBuffer buffer(1024);

	std::unique_ptr<PacketBuffer> buffer_ptr = std::make_unique<PacketBuffer>(std::move(buffer));

	buffer_pool.push(std::move(buffer_ptr));

	// Check the buffer was added back
	REQUIRE(buffer_pool.getBufferCount() == 1);
}

TEST_CASE("Check that we get an exception adding an incorrectly sized buffer to the pool", "[buffers]") {
	accl::BufferPool<PacketBuffer> buffer_pool = accl::BufferPool<PacketBuffer>(1024, 1);

	PacketBuffer buffer(100);

	std::unique_ptr<PacketBuffer> buffer_ptr = std::make_unique<PacketBuffer>(std::move(buffer));

	SECTION("Expect a invalid_argument is thrown if we try add a buffer of incorrect size") {
		REQUIRE_THROWS_AS(buffer_pool.push(std::move(buffer_ptr)), std::invalid_argument);
	}

	// Check the buffer was added back
	REQUIRE(buffer_pool.getBufferCount() == 1);
}

TEST_CASE("Check that we get an exception adding an incorrectly sized buffers to the pool", "[buffers]") {
	accl::BufferPool<PacketBuffer> buffer_pool = accl::BufferPool<PacketBuffer>(1024, 1);

	std::deque<std::unique_ptr<PacketBuffer>> buffers;

	auto buffer_ptr = std::make_unique<PacketBuffer>(1000);
	buffers.push_back(std::move(buffer_ptr));

	SECTION("Expect a invalid_argument is thrown if we try add buffers of incorrect size") {
		REQUIRE_THROWS_AS(buffer_pool.push(buffers), std::invalid_argument);
	}

	// Check the buffer was added back
	REQUIRE(buffer_pool.getBufferCount() == 1);
}

TEST_CASE("Check we get all the buffers when we popAll()", "[buffers]") {
	accl::BufferPool<PacketBuffer> buffer_pool = accl::BufferPool<PacketBuffer>(1024, 5);

	auto buffers = buffer_pool.pop(accl::BUFFER_POOL_POP_ALL);

	// Check we got 5 buffers
	REQUIRE(buffers.size() == 5);

	// Also check that buffer_pool is now empty
	REQUIRE(buffer_pool.getBufferCount() == 0);
}

TEST_CASE("Check we can push multiple buffers back into the pool", "[buffers]") {
	accl::BufferPool<PacketBuffer> buffer_pool = accl::BufferPool<PacketBuffer>(1024, 5);

	auto buffers = buffer_pool.pop(accl::BUFFER_POOL_POP_ALL);

	// Check we got 5 buffers
	REQUIRE(buffers.size() == 5);

	// Add buffers back
	buffer_pool.push(buffers);

	// Check buffers is empty
	REQUIRE(buffers.size() == 0);

	// Check the buffers were added back
	REQUIRE(buffer_pool.getBufferCount() == 5);
}
