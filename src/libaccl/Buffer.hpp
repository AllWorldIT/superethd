/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint> // for uint8_t
#include <stdexcept>
#include <vector>

namespace accl {

class Buffer {
	private:
		std::vector<uint8_t> content;
		std::size_t dataSize;

	public:
		Buffer(std::size_t size);

		// Add data to buffer
		void append(const uint8_t *data, std::size_t size);

		// Get pointer to data
		const uint8_t *getData() const;

		// Get size of buffer
		std::size_t getBufferSize() const;

		// Get amount of data in buffer
		std::size_t getDataSize() const;
		// Set amount of data in buffer
		void setDataSize(size_t size);

		// Clear the buffer
		void clear();
};

} // namespace accl