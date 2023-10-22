/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "Buffer.hpp"
#include <condition_variable>
#include <iostream>
#include <list>
#include <memory>
#include <shared_mutex>
#include <thread>

namespace accl {

class BufferPool {
	private:
		std::list<std::unique_ptr<Buffer>> pool;
		std::size_t buffer_size;
		std::shared_mutex mtx;
		std::condition_variable_any cv;

	public:
		BufferPool(std::size_t buffer_size);
		BufferPool(std::size_t buffer_size, std::size_t num_buffers);

		~BufferPool();

		std::unique_ptr<Buffer> pop();
		std::vector<std::unique_ptr<Buffer>> popAll();
		std::vector<std::unique_ptr<Buffer>> popMany(size_t count);
		void push(std::unique_ptr<Buffer> buffer);

		size_t getBufferCount();

		void listener();
};

} // namespace accl