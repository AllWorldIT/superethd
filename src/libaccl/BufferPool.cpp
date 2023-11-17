/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "BufferPool.hpp"
#include "Buffer.hpp"
#include <chrono>
#include <sstream>

namespace accl {

// Internal method to pop many buffers from the pool WITHOUT LOCKING
void BufferPool::_pop(std::deque<std::unique_ptr<Buffer>> &result, size_t count) {
	// Grab iterator on first buffer in pool
	auto iterator = pool.begin();
	// We now look while count==0, or i < count, making sure we've not reached the pool end
	for (size_t i = 0; (!count || i < count) && iterator != pool.end(); ++i, ++iterator) {
		result.push_back(std::move(*iterator));
	}

	// Erase the buffers that we moved from the list
	pool.erase(pool.begin(), iterator);
}

BufferPool::BufferPool(std::size_t buffer_size, std::size_t num_buffers) : BufferPool(buffer_size) {
	// Allocate the number of buffers specified
	for (std::size_t i = 0; i < num_buffers; ++i) {
		auto buffer = std::make_unique<Buffer>(buffer_size);
		pool.push_back(std::move(buffer));
	}
}

// Internal method to pop many buffers from the pool WITHOUT LOCKING
std::deque<std::unique_ptr<Buffer>> BufferPool::_pop(size_t count) {
	std::deque<std::unique_ptr<Buffer>> result;
	_pop(result, count);
	return result;
}

/**
 * @brief Pop a single buffer from the pool.
 *
 * @return std::unique_ptr<Buffer> Buffer popped from the pool.
 * @exception std::bad_alloc No buffers were available to pop.
 */
std::unique_ptr<Buffer> BufferPool::pop() {
	std::unique_lock<std::shared_mutex> lock(mtx);
	if (pool.empty()) {
		throw std::bad_alloc();
	}
	auto buffer = std::move(pool.front());
	pool.pop_front();
	return buffer;
}

/**
 * @brief Pop buffers from the pool.
 *
 * @param count Number of buffers to pop from pool, using a count of `accl::BUFFER_POOL_POP_ALL` will pop all buffers.
 * @return std::deque<std::unique_ptr<Buffer>> Vector of popped buffers.
 */
std::deque<std::unique_ptr<Buffer>> BufferPool::pop(size_t count) {
	std::unique_lock<std::shared_mutex> lock(mtx);
	return _pop(count);
}

void BufferPool::push(std::unique_ptr<Buffer> buffer) {
	// Make sure buffer size matches
	if (buffer->getBufferSize() != buffer_size) {
		std::ostringstream oss;
		oss << "Buffer is of incorrect size " << buffer->getBufferSize() << " (buffer) vs. " << buffer_size << " (pool)";
		throw std::invalid_argument(oss.str());
	}
	std::unique_lock<std::shared_mutex> lock(mtx);
	pool.push_back(std::move(buffer));

	cv.notify_one(); // Notify listener thread
}

void BufferPool::push(std::deque<std::unique_ptr<Buffer>> &buffers) {
	// We can check all the buffers before doing the lock to save on how long we hold the lock
	for (auto &buffer : buffers) {
		// Make sure buffer size matches
		if (buffer->getBufferSize() != buffer_size) {
			std::ostringstream oss;
			oss << "Buffer is of incorect size " << buffer->getBufferSize() << " vs. " << buffer_size;
			throw std::invalid_argument(oss.str());
		}
	}
	// Lock the pool
	std::unique_lock<std::shared_mutex> lock(mtx);
	// Move all buffers into the pool in one go
	std::move(buffers.begin(), buffers.end(), std::back_inserter(pool));
	// Clear the old buffer list
	buffers.clear();
	cv.notify_one(); // Notify listener threa
}

size_t BufferPool::getBufferCount() {
	std::shared_lock<std::shared_mutex> lock(mtx);
	return pool.size();
}

void BufferPool::wait(std::deque<std::unique_ptr<Buffer>> &results) {
	std::unique_lock<std::shared_mutex> lock(mtx);
	// We only really need to wait if the pool is empty
	if (pool.empty())
		cv.wait(lock);
	_pop(results, BUFFER_POOL_POP_ALL);
}

std::deque<std::unique_ptr<Buffer>> BufferPool::wait() {
	std::deque<std::unique_ptr<Buffer>> results;
	wait(results);
	return results;
}


bool BufferPool::wait_for(std::chrono::milliseconds duration, std::deque<std::unique_ptr<Buffer>> &results) {
	std::unique_lock<std::shared_mutex> lock(mtx);

	// If we have items we don't have to wait
	if (!pool.empty()) {
		_pop(results, BUFFER_POOL_POP_ALL);
		return true;
	}

	// If we have a zero tick count, wait indefinitely
	if (!duration.count()) {
		cv.wait(lock);
		_pop(results, BUFFER_POOL_POP_ALL);
		return true;
	}

	// Wait for a duration
	if (cv.wait_for(lock, duration) == std::cv_status::no_timeout) {
		_pop(results, BUFFER_POOL_POP_ALL);
		return true;
	}

	return false;
}

std::deque<std::unique_ptr<Buffer>> BufferPool::wait_for(std::chrono::milliseconds duration) {
	std::deque<std::unique_ptr<Buffer>> results;
	wait_for(duration, results);
	return results;
}


} // namespace accl