/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "Buffer.hpp"

namespace accl {

Buffer::Buffer(std::size_t size) : content(size), dataSize(0) {}

// Add data to buffer
void Buffer::append(const uint8_t *data, std::size_t size) {
	if (size > content.size() - dataSize) {
		// Make sure we cannot overflow the buffer
		throw std::out_of_range("Buffer size would be exceeded with append");
	}

	std::copy(data, data + size, content.begin() + dataSize);
	dataSize += size;
}

// Get pointer to data
const uint8_t *Buffer::getData() const { return content.data(); }

// Get size of buffer
std::size_t Buffer::getBufferSize() const { return content.size(); }

// Get amount of useful data in buffer
std::size_t Buffer::getDataSize() const { return dataSize; }
// Set amount of data that is in the buffer
void Buffer::setDataSize(size_t size) {
	// We cannot set the data size bigger than the buffer itself
	if (size > content.size()) {
		throw std::out_of_range("Buffer data size cannot exceed buffer size");
	}
	dataSize = size;

}

// Clear the buffer
void Buffer::clear() { dataSize = 0; }

} // namespace accl