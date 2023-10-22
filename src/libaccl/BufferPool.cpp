/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "BufferPool.hpp"
#include <sstream>

namespace accl {

BufferPool::BufferPool(std::size_t buffer_size) : buffer_size(buffer_size) {}

BufferPool::BufferPool(std::size_t buffer_size, std::size_t num_buffers) : BufferPool(buffer_size) {
	// Allocate the number of buffers specified
	for (std::size_t i = 0; i < num_buffers; ++i) {
		auto buffer = std::make_unique<Buffer>(buffer_size);
		pool.push_back(std::move(buffer));
	}
}

BufferPool::~BufferPool() {
	// Buffers should be automagically freed
}

std::unique_ptr<Buffer> BufferPool::pop() {
	std::unique_lock<std::shared_mutex> lock(mtx);
	if (pool.empty()) {
		throw std::length_error("BufferPool empty");
	}
	auto buffer = std::move(pool.front());
	pool.pop_front();
	return buffer;
}

// Return all buffers
std::vector<std::unique_ptr<Buffer>> BufferPool::popAll() { return popMany(0); }

// Return many buffers
std::vector<std::unique_ptr<Buffer>> BufferPool::popMany(size_t count) {
	std::vector<std::unique_ptr<Buffer>> result;
	// Lock the pool
	std::unique_lock<std::shared_mutex> lock(mtx);

	// Grab iterator on first buffer in pool
	auto iterator = pool.begin();
	// We now look while count==0, or i < count, making sure we've not reached the pool end
	for (size_t i = 0; (!count || i < count) && iterator != pool.end(); ++i, ++iterator) {
		result.push_back(std::move(*iterator));
	}

	// Erase the buffers that we moved from the list
	pool.erase(pool.begin(), iterator);

	return result;
}

void BufferPool::push(std::unique_ptr<Buffer> buffer) {
	// Make sure buffer size matches
	if (buffer->getBufferSize() != buffer_size) {
		std::ostringstream oss;
		oss << "Buffer is of incorect size " << buffer->getBufferSize() << " vs. " << buffer_size;
		throw std::invalid_argument(oss.str());
	}
	std::unique_lock<std::shared_mutex> lock(mtx);
	pool.push_back(std::move(buffer));
	cv.notify_one(); // Notify listener thread
}

size_t BufferPool::getBufferCount() {
	std::shared_lock<std::shared_mutex> lock(mtx);
	return pool.size();
}

void BufferPool::listener() {
	std::shared_lock<std::shared_mutex> lock(mtx);
	while (true) {
		cv.wait(lock); // Wait for notification
		std::cout << "A buffer was allocated.\n";
		// Do additional work if necessary...
	}
}

} // namespace accl