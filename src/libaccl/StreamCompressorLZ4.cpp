/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "StreamCompressorLZ4.hpp"
#include <stdexcept>

extern "C" {
#include <lz4.h>
}

namespace accl {

StreamCompressorLZ4::StreamCompressorLZ4() : StreamCompressor() {
	// Create stream tracking contexts
	if (!(lz4Stream = LZ4_createStream())) {
		throw std::runtime_error("Failed to create LZ4 stream");
	}
	if (!(lz4StreamDecode = LZ4_createStreamDecode())) {
		throw std::runtime_error("Failed to create LZ4 stream decode");
	}
}

StreamCompressorLZ4::~StreamCompressorLZ4() {
	// Free stream tracking contexts
	LZ4_freeStreamDecode(lz4StreamDecode);
	LZ4_freeStream(lz4Stream);
}

void StreamCompressorLZ4::resetCompressionStream() { LZ4_resetStream_fast(lz4Stream); }
void StreamCompressorLZ4::resetDecompressionStream() { LZ4_setStreamDecode(lz4StreamDecode, NULL, 0); }

int StreamCompressorLZ4::compress(const char *input, size_t input_size, char *output, size_t max_output_size) {
	//	return LZ4_compress_default(input, output, static_cast<int>(input_size), static_cast<int>(max_output_size));
	return LZ4_compress_fast_continue(lz4Stream, input, output, static_cast<int>(input_size), static_cast<int>(max_output_size),
									  compression_level);
}

int StreamCompressorLZ4::decompress(const char *input, size_t input_size, char *output, size_t max_output_size) {
	//	return LZ4_decompress_safe(input, output, static_cast<int>(input_size), static_cast<int>(max_output_size));
	return LZ4_decompress_safe_continue(lz4StreamDecode, input, output, static_cast<int>(input_size),
										static_cast<int>(max_output_size));
}

}