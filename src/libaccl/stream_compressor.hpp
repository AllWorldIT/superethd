/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef>
#include <string>

namespace accl {

class StreamCompressor {
	protected:
		int compression_level;

	public:
		StreamCompressor();
		virtual ~StreamCompressor();

		virtual void resetCompressionStream() = 0;
		virtual void resetDecompressionStream() = 0;

		virtual int compress(const char *input, size_t input_size, char *output, size_t max_output_size) = 0;
		virtual int decompress(const char *input, size_t input_size, char *output, size_t max_output_size) = 0;

		virtual const std::string strerror(int err) = 0;
};

} // namespace accl