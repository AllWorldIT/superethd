/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Logger.hpp"
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>


namespace accl {

class Buffer {
	private:
		std::vector<char> content;
		std::size_t dataSize;

	public:
		inline Buffer(std::size_t size) : content(size), dataSize(0){};

		// Add data to buffer
		inline void append(const char *data, std::size_t size);

		// Get pointer to data
		inline char *getData();

		// Get size of buffer
		inline std::size_t getBufferSize() const;

		// Get amount of data in buffer
		inline std::size_t getDataSize() const;
		// Set amount of data in buffer
		inline void setDataSize(size_t size);

		// Clear the buffer
		inline void clear();
};

// Add data to buffer
inline void Buffer::append(const char *data, std::size_t size) {
	if (size > content.size() - dataSize) {
		// Make sure we cannot overflow the buffer
		throw std::out_of_range("Buffer size would be exceeded with append");
	}
	std::copy(data, data + size, content.begin() + dataSize);
	dataSize += size;
}

// Return pointer to the data
inline char *Buffer::getData() { return content.data(); }

// Get buffer size
inline std::size_t Buffer::getBufferSize() const { return content.size(); };

// Return current data size within the buffer
inline std::size_t Buffer::getDataSize() const { return dataSize; }

// Set amount of data that is in the buffer
inline void Buffer::setDataSize(size_t size) {
	// We cannot set the data size bigger than the buffer itself
	if (size > content.size()) {
		throw std::out_of_range("Buffer data size cannot exceed buffer size");
	}
	dataSize = size;
}

// Clear buffer data
inline void Buffer::clear() { dataSize = 0; }

} // namespace accl