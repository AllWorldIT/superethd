/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "Buffer.hpp"

namespace accl {

Buffer::Buffer(std::size_t size) : buffer(size), dataSize(0) {}

// Add data to buffer
void Buffer::addData(const uint8_t *data, std::size_t size) {
	if (size > buffer.size() - dataSize) {
		// Handle overflow
		throw std::length_error("Exceeded maximum buffer size");
	}

	std::copy(data, data + size, buffer.begin() + dataSize);
	dataSize += size;
}

// Get pointer to data
const uint8_t *Buffer::getData() const { return buffer.data(); }

// Get size of buffer
std::size_t Buffer::getBufferSize() const { return buffer.size(); }

// Get amount of useful data in buffer
std::size_t Buffer::getDataSize() const { return dataSize; }

// Clear the buffer
void Buffer::clear() { dataSize = 0; }

} // namespace accl