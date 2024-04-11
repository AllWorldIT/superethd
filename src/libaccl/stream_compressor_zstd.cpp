/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "stream_compressor_zstd.hpp"
#include <stdexcept>

extern "C" {
#include <zstd.h>
}

namespace accl {

StreamCompressorZSTD::StreamCompressorZSTD() : StreamCompressor() {
	// Create stream tracking contexts
	if (!(cctx = ZSTD_createCCtx())) {
		throw std::runtime_error("Failed to create ZSTD compression context");
	}
	if (!(dctx = ZSTD_createDCtx())) {
		throw std::runtime_error("Failed to create ZSTD decompression context");
	}
}

StreamCompressorZSTD::~StreamCompressorZSTD() {
	// Free stream tracking contexts
	ZSTD_freeDCtx(dctx);
	ZSTD_freeCCtx(cctx);
}

void StreamCompressorZSTD::resetCompressionStream() { ZSTD_CCtx_reset(cctx, ZSTD_reset_session_and_parameters); }
void StreamCompressorZSTD::resetDecompressionStream() { ZSTD_DCtx_reset(dctx, ZSTD_reset_session_and_parameters); }

int StreamCompressorZSTD::compress(const char *input, size_t input_size, char *output, size_t max_output_size) {
	//	return ZSTD_compress_default(input, output, static_cast<int>(input_size), static_cast<int>(max_output_size));
	return ZSTD_compressCCtx(cctx, output, static_cast<int>(max_output_size), input, static_cast<int>(input_size),
							 compression_level);
}

int StreamCompressorZSTD::decompress(const char *input, size_t input_size, char *output, size_t max_output_size) {
	return ZSTD_decompressDCtx(dctx, output, static_cast<int>(max_output_size), input, static_cast<int>(input_size));
}

const std::string StreamCompressorZSTD::strerror(int err) { return ZSTD_getErrorName(err); }

} // namespace accl
