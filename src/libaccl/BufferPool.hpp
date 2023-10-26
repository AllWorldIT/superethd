/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Buffer.hpp"
#include <condition_variable>
#include <iostream>
#include <list>
#include <memory>
#include <new>
#include <shared_mutex>
#include <thread>

namespace accl {

// Inline constant to pass to pop() to allow popping all buffers
inline constexpr size_t BUFFER_POOL_POP_ALL = 0;

class BufferPool {
	private:
		std::list<std::unique_ptr<Buffer>> pool;
		std::size_t buffer_size;
		std::shared_mutex mtx;
		std::condition_variable_any cv;

		void _pop(std::vector<std::unique_ptr<Buffer>> &result, size_t count);
		inline std::vector<std::unique_ptr<Buffer>> _pop(size_t count);

	public:
		inline BufferPool(std::size_t buffer_size) : buffer_size(buffer_size){};
		BufferPool(std::size_t buffer_size, std::size_t num_buffers);

		~BufferPool() = default;

		std::unique_ptr<Buffer> pop();
		inline std::vector<std::unique_ptr<Buffer>> pop(size_t count);

		void push(Buffer &&buffer);
		void push(std::unique_ptr<Buffer> &buffer);
		void push(std::vector<std::unique_ptr<Buffer>> &buffers);

		inline size_t getBufferCount();

		std::vector<std::unique_ptr<Buffer>> wait();
		bool wait_for(std::chrono::seconds duration, std::vector<std::unique_ptr<Buffer>> &result);
};

// Internal method to pop many buffers from the pool WITHOUT LOCKING
inline std::vector<std::unique_ptr<Buffer>> BufferPool::_pop(size_t count) {
	std::vector<std::unique_ptr<Buffer>> result;
	_pop(result, count);
	return result;
}

/**
 * @brief Pop buffers from the pool.
 *
 * @param count Number of buffers to pop from pool, using a count of `accl::BUFFER_POOL_POP_ALL` will pop all buffers.
 * @return std::vector<std::unique_ptr<Buffer>> Vector of popped buffers.
 */
inline std::vector<std::unique_ptr<Buffer>> BufferPool::pop(size_t count) {
	std::unique_lock<std::shared_mutex> lock(mtx);
	return _pop(count);
}

inline size_t BufferPool::getBufferCount() {
	std::shared_lock<std::shared_mutex> lock(mtx);
	return pool.size();
}

} // namespace accl