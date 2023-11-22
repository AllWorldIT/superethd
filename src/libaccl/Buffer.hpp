/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Logger.hpp"
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

/**
 * @brief Append data to buffer.
 *
 * @param data Pointer to data.
 * @param size Size of data to.
 */
inline void Buffer::append(const char *data, std::size_t size) {
	if (size > content.size() - dataSize) {
		// Make sure we cannot overflow the buffer
		throw std::out_of_range(std::format("Buffer size {} would be exceeded with append of {} ontop of current size {}",
											content.size(), size, dataSize));
	}
	std::copy(data, data + size, content.begin() + dataSize);
	dataSize += size;
}

/**
 * @brief Get pointer to buffer data.
 *
 * @return char* Pointer to buffer data.
 */
inline char *Buffer::getData() { return content.data(); }

/**
 * @brief Get buffer size.
 *
 * @return std::size_t Buffer size.
 */
inline std::size_t Buffer::getBufferSize() const { return content.size(); };

/**
 * @brief Get size of data in buffer.
 *
 * @return std::size_t Size of data in buffer.
 */
inline std::size_t Buffer::getDataSize() const { return dataSize; }

/**
 * @brief Set size of data in buffer.
 *
 * @param size Size of data.
 */
inline void Buffer::setDataSize(size_t size) {
	// We cannot set the data size bigger than the buffer itself
	if (size > content.size()) {
		throw std::out_of_range("Buffer data size cannot exceed buffer size");
	}
	dataSize = size;
}

/**
 * @brief Clear data in buffer by setting the size to 0.
 *
 */
inline void Buffer::clear() { dataSize = 0; }

} // namespace accl