/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "StreamCompressor.hpp"

extern "C" {
#include <lz4.h>
}

namespace accl {

class StreamCompressorLZ4 : public StreamCompressor {
		LZ4_stream_t *lz4Stream;
		LZ4_streamDecode_t *lz4StreamDecode;

	public:
		StreamCompressorLZ4();

		~StreamCompressorLZ4() override;

		void resetCompressionStream() override;
		void resetDecompressionStream() override;

		int compress(const char *input, size_t input_size, char *output, size_t max_output_size) override;

		int decompress(const char *input, size_t input_size, char *output, size_t max_output_size) override;

		const std::string strerror(int err) override;
};

}