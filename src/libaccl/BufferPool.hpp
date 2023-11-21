/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Buffer.hpp"
#include <condition_variable>
#include <deque>
#include <shared_mutex>

namespace accl {

// Inline constant to pass to pop() to allow popping all buffers
inline constexpr size_t BUFFER_POOL_POP_ALL{0};

template <typename T> class BufferPool {
	private:
		std::deque<std::unique_ptr<T>> pool;
		std::size_t buffer_size;
		std::shared_mutex mtx;
		std::condition_variable_any cv;

		void _pop(std::deque<std::unique_ptr<T>> &result, size_t count);
		std::deque<std::unique_ptr<T>> _pop(size_t count);

	public:
		BufferPool(std::size_t buffer_size) : buffer_size(buffer_size){};
		BufferPool(std::size_t buffer_size, std::size_t num_buffers);

		~BufferPool() = default;

		std::unique_ptr<T> pop();
		std::deque<std::unique_ptr<T>> pop(size_t count);
		std::unique_ptr<T> pop_wait();

		void push(T &buffer);
		void push(std::unique_ptr<T> buffer);
		void push(std::deque<std::unique_ptr<T>> &buffers);

		size_t getBufferCount();

		void wait(std::deque<std::unique_ptr<T>> &results);
		std::deque<std::unique_ptr<T>> wait();

		bool wait_for(std::chrono::milliseconds duration, std::deque<std::unique_ptr<T>> &results);
		std::deque<std::unique_ptr<T>> wait_for(std::chrono::milliseconds duration);
};

/**
 * @brief Internal method to pop a number of buffers or all buffers without locking.
 *
 * @tparam T Buffer class.
 * @param result Result to place buffers in which are popped.
 * @param count Number of buffers to pop or BUFFER_POOL_POP_ALL for all of them.
 */
template <typename T> void BufferPool<T>::_pop(std::deque<std::unique_ptr<T>> &result, size_t count) {
	// Grab iterator on first buffer in pool
	auto iterator = pool.begin();

	// We now look while count==0, or i < count, making sure we've not reached the pool end
	for (size_t i = 0; (!count || i < count) && iterator != pool.end(); ++i, ++iterator) {
		result.push_back(std::move(*iterator));
	}

	// Erase the buffers that we moved from the list
	pool.erase(pool.begin(), iterator);
}

/**
 * @brief Internal method to pop a number of buffers without locking.
 *
 * @tparam T Buffer class.
 * @param count Number of buffers to pop.
 * @return std::deque<std::unique_ptr<T>> Buffers that were popped.
 */
template <typename T> std::deque<std::unique_ptr<T>> BufferPool<T>::_pop(size_t count) {
	std::deque<std::unique_ptr<T>> result;
	_pop(result, count);
	return result;
}

/**
 * @brief Construct a new Buffer Pool< T>:: Buffer Pool object
 *
 * @tparam T Buffer class.
 * @param buffer_size Size of the buffers we'll be using.
 * @param num_buffers Number of buffers to place in the pool apon construction.
 */
template <typename T> BufferPool<T>::BufferPool(std::size_t buffer_size, std::size_t num_buffers) : BufferPool(buffer_size) {
	// Allocate the number of buffers specified
	for (std::size_t i = 0; i < num_buffers; ++i) {
		auto buffer = std::make_unique<T>(buffer_size);
		pool.push_back(std::move(buffer));
	}
}

/**
 * @brief Pop a single buffer from the pool.
 *
 * @return std::unique_ptr<T> Buffer popped from the pool.
 * @exception std::bad_alloc No buffers were available to pop.
 */
template <typename T> std::unique_ptr<T> BufferPool<T>::pop() {
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
 * @return std::deque<std::unique_ptr<T>> Vector of popped buffers.
 */
template <typename T> std::deque<std::unique_ptr<T>> BufferPool<T>::pop(size_t count) {
	std::unique_lock<std::shared_mutex> lock(mtx);
	return _pop(count);
}

/**
 * @brief Pop a single buffer from the pool, waiting if there are none available.
 *
 * @return std::unique_ptr<T> Buffer popped from the pool.
 * @exception std::bad_alloc No buffers were available to pop.
 */
template <typename T> std::unique_ptr<T> BufferPool<T>::pop_wait() {
	std::unique_lock<std::shared_mutex> lock(mtx);
	// We only need to wait if the pool is empty
	if (pool.empty())
		cv.wait(lock);
	auto buffer = std::move(pool.front());
	pool.pop_front();
	return buffer;
}

/**
 * @brief Push a buffer into the pool
 *
 * @tparam T Buffer class.
 * @param buffer Buffer to push into the pool.
 */
template <typename T> void BufferPool<T>::push(std::unique_ptr<T> buffer) {
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

/**
 * @brief Push a number of buffers into the pool.
 *
 * @tparam T Buffer class.
 * @param buffers Buffers to push into the pool.
 */
template <typename T> void BufferPool<T>::push(std::deque<std::unique_ptr<T>> &buffers) {
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

/**
 * @brief Get the number of buffers in the pool.
 *
 * @tparam T Buffer class.
 * @return size_t Number of buffers in the pool.
 */
template <typename T> size_t BufferPool<T>::getBufferCount() {
	std::shared_lock<std::shared_mutex> lock(mtx);
	return pool.size();
}

/**
 * @brief Wait for the pool to get buffers and push them into a results list.
 *
 * @tparam T Buffer class.
 * @param results Resuts list to push buffers into.
 */
template <typename T> void BufferPool<T>::wait(std::deque<std::unique_ptr<T>> &results) {
	std::unique_lock<std::shared_mutex> lock(mtx);
	// We only really need to wait if the pool is empty
	if (pool.empty())
		cv.wait(lock);
	_pop(results, BUFFER_POOL_POP_ALL);
}

/**
 * @brief Wait for a single buffer to be available in the pool.
 *
 * @tparam T Buffer class.
 * @return std::deque<std::unique_ptr<T>>
 */
template <typename T> std::deque<std::unique_ptr<T>> BufferPool<T>::wait() {
	std::deque<std::unique_ptr<T>> results;
	wait(results);
	return results;
}

/**
 * @brief Wait for a duration and push the buffers into a results list.
 *
 * @tparam T Buffer class.
 * @param duration Duration in milliseconds to wait.
 * @param results Results list.
 * @return true If we got buffers.
 * @return false If we timed out.
 */
template <typename T> bool BufferPool<T>::wait_for(std::chrono::milliseconds duration, std::deque<std::unique_ptr<T>> &results) {
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

/**
 * @brief Wait for a duration and return the buffers we got.
 *
 * @tparam T Buffer class.
 * @param duration Duration to wait in milliseconds.
 * @return std::deque<std::unique_ptr<T>> List of buffers returned.
 */
template <typename T> std::deque<std::unique_ptr<T>> BufferPool<T>::wait_for(std::chrono::milliseconds duration) {
	std::deque<std::unique_ptr<T>> results;
	wait_for(duration, results);
	return results;
}

} // namespace accl
