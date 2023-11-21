/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef>

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
};

}