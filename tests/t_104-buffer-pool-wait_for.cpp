/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "libtests/framework.hpp"

void wait_worker(accl::BufferPool &buffer_pool) {
	const std::string test_string = "hello world";

	// Create our buffer pool to hold the result
	std::vector<std::unique_ptr<accl::Buffer>> buffers;

	// Wait for buffer pool to get a buffer
	bool res = buffer_pool.wait_for(std::chrono::seconds(1), buffers);
	// The first call should be false (timed out)
	assert(res == false);
	// Make sure the vector is empty
	assert(buffers.size() == 0);

	// Wait for buffer pool to get a buffer
	res = buffer_pool.wait_for(std::chrono::seconds(5), buffers);
	// The second call should have not timed out
	assert(res == true);
	// The vector size should now be 1
	assert(buffers.size() == 1);

	// Pull the buffer from the vector returned
	auto first_buffer = buffers.begin();
	auto buffer = std::move(*first_buffer);
	buffers.erase(first_buffer);

	// Convert buffer into a string
	std::string buffer_string(reinterpret_cast<const char *>(buffer->getData()), buffer->getDataSize());

	// Lets compare it to make sure its correct
	assert(test_string == buffer_string);
}

void push_worker(accl::BufferPool &buffer_pool) {
	const std::string test_string = "hello world";

	// Wait until the wait worker has started
	std::this_thread::sleep_for(std::chrono::seconds(2));

	// Grab a buffer from the pool
	auto buffer = buffer_pool.pop();

	// Add some data into the buffer
	buffer->append(reinterpret_cast<const uint8_t *>(test_string.data()), test_string.length());

	// Now add the buffer back to the pool, which should trigger the wait_worker
	buffer_pool.push(buffer);
}

TEST_CASE("Check that pushing a buffer into the pool triggers the timed waiter", "[buffers]") {
	accl::BufferPool buffer_pool = accl::BufferPool(12, 1);

	std::thread t1(wait_worker, std::ref(buffer_pool));
	std::thread t2(push_worker, std::ref(buffer_pool));

	t1.join();
	t2.join();
}
