/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "libtests/framework.hpp"

void wait_worker(accl::BufferPool<PacketBuffer> &buffer_pool) {
	const std::string test_string = "hello world";

	// Wait for buffer pool to get a buffer
	auto buffers = buffer_pool.wait();

	// We should have one buffer returned from the wait
	REQUIRE(buffers.size() == 1);

	// Pull the buffer from the vector returned
	auto first_buffer = buffers.begin();
	auto buffer = std::move(*first_buffer);
	buffers.erase(first_buffer);

	// Convert buffer into a string
	std::string buffer_string(reinterpret_cast<const char *>(buffer->getData()), buffer->getDataSize());

	// Lets compare it to make sure its correct
	REQUIRE(test_string == buffer_string);
}

void push_worker(accl::BufferPool<PacketBuffer> &buffer_pool) {
	const std::string test_string = "hello world";

	// Wait until the wait worker has started
	std::this_thread::sleep_for(std::chrono::seconds(2));

	// Create a buffer
	std::unique_ptr<PacketBuffer> buffer = std::make_unique<PacketBuffer>(12);

	// Add some data into the buffer
	buffer->append(reinterpret_cast<const char *>(test_string.data()), test_string.length());

	// Now add the buffer back to the pool, which should trigger the wait_worker
	buffer_pool.push(std::move(buffer));
}

TEST_CASE("Check that pushing a buffer into the pool triggers the waiter", "[buffers]") {
	accl::BufferPool<PacketBuffer> buffer_pool = accl::BufferPool<PacketBuffer>(12);

	std::thread t1(wait_worker, std::ref(buffer_pool));
	std::thread t2(push_worker, std::ref(buffer_pool));

	t1.join();
	t2.join();
}
