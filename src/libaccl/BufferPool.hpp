/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Buffer.hpp"
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <list>
#include <memory>
#include <new>
#include <shared_mutex>
#include <thread>

namespace accl {

// Inline constant to pass to pop() to allow popping all buffers
inline constexpr size_t BUFFER_POOL_POP_ALL{0};

class BufferPool {
	private:
		std::list<std::unique_ptr<Buffer>> pool;
		std::size_t buffer_size;
		std::shared_mutex mtx;
		std::condition_variable_any cv;

		void _pop(std::vector<std::unique_ptr<Buffer>> &result, size_t count);
		std::vector<std::unique_ptr<Buffer>> _pop(size_t count);

	public:
		BufferPool(std::size_t buffer_size) : buffer_size(buffer_size){};
		BufferPool(std::size_t buffer_size, std::size_t num_buffers);

		~BufferPool() = default;

		std::unique_ptr<Buffer> pop();
		std::vector<std::unique_ptr<Buffer>> pop(size_t count);

		void push(Buffer &buffer);
		void push(std::unique_ptr<Buffer> buffer);
		void push(std::vector<std::unique_ptr<Buffer>> &buffers);

		size_t getBufferCount();

		void wait(std::vector<std::unique_ptr<Buffer>> &results);
		std::vector<std::unique_ptr<Buffer>> wait();

		bool wait_for(std::chrono::milliseconds duration, std::vector<std::unique_ptr<Buffer>> &results);
		std::vector<std::unique_ptr<Buffer>> wait_for(std::chrono::milliseconds duration);
};





} // namespace accl