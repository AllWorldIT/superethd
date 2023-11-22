/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "libtests/framework.hpp"

void wait_worker(accl::BufferPool<PacketBuffer> &buffer_pool) {
	const std::string test_string = "hello world";

	// Create our buffer pool to hold the result
	std::deque<std::unique_ptr<PacketBuffer>> buffers;

	// Wait for buffer pool to get a buffer
	bool res = buffer_pool.wait_for(std::chrono::seconds(1), buffers);
	// The first call should be false (timed out)
	REQUIRE(res == false);
	// Make sure the deque is empty
	REQUIRE(buffers.size() == 0);

	// Wait for buffer pool to get a buffer
	res = buffer_pool.wait_for(std::chrono::seconds(5), buffers);
	// The second call should have not timed out
	REQUIRE(res == true);
	// The deque size should now be 1
	REQUIRE(buffers.size() == 1);

	// Pull the buffer from the deque returned
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

TEST_CASE("Check that pushing a buffer into the pool triggers the timed waiter", "[buffers]") {
	accl::BufferPool<PacketBuffer> buffer_pool = accl::BufferPool<PacketBuffer>(12);

	std::thread t1(wait_worker, std::ref(buffer_pool));
	std::thread t2(push_worker, std::ref(buffer_pool));

	t1.join();
	t2.join();
}
