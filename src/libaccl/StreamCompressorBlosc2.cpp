/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "StreamCompressorBlosc2.hpp"

extern "C" {
#include <blosc2.h>
}

namespace accl {

StreamCompressorBlosc2::StreamCompressorBlosc2() : StreamCompressor() {
	blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
	cparams.typesize = 1;
	cparams.compcode = BLOSC_BLOSCLZ;
	cparams.clevel = compression_level;
	cparams.nthreads = 1;

	blosc2_dparams dparams = BLOSC2_DPARAMS_DEFAULTS;
	dparams.nthreads = 1;

	cctx = blosc2_create_cctx(cparams);
	dctx = blosc2_create_dctx(dparams);
}

StreamCompressorBlosc2::~StreamCompressorBlosc2() {
	blosc2_free_ctx(cctx);
	blosc2_free_ctx(dctx);
}

void StreamCompressorBlosc2::resetCompressionStream() {}
void StreamCompressorBlosc2::resetDecompressionStream() {}

int StreamCompressorBlosc2::compress(const char *input, size_t input_size, char *output, size_t max_output_size) {
	return blosc2_compress_ctx(cctx, input, input_size, output, max_output_size);
}

int StreamCompressorBlosc2::decompress(const char *input, size_t input_size, char *output, size_t max_output_size) {
	return blosc2_decompress_ctx(dctx, input, input_size, output, max_output_size);
}

} // namespace accl
