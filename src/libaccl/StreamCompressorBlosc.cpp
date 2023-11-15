/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "StreamCompressorBlosc.hpp"

extern "C" {
	#include <blosc2.h>
}

CompressorBlosc::CompressorBlosc() {
	blosc2_init();
	blosc2_create_cctx(cctx);
	dctx = blosc2_create_dctx(nullptr);
}

CompressorBlosc::~CompressorBlosc() {
	blosc2_free_ctx(cctx);
	blosc2_free_ctx(dctx);
	blosc_finalize();
}

int CompressorBlosc::compress(const char *input, size_t input_size, char *output, size_t max_output_size) {
	return blosc2_compress_ctx(cctx, sizeof(char), input_size, input, output, max_output_size);
}

int CompressorBlosc::decompress(const char *input, size_t input_size, char *output, size_t max_output_size) {
	return blosc2_decompress_ctx(dctx, input, input_size, output, max_output_size);
}
