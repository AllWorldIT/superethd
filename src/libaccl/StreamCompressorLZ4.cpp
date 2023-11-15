/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "StreamCompressorLZ4.hpp"

extern "C" {
#include <lz4.h>
}

int CompressorLZ4::compress(const char *input, size_t input_size, char *output, size_t max_output_size) {
	return LZ4_compress_default(input, output, static_cast<int>(input_size), static_cast<int>(max_output_size));
}

int CompressorLZ4::decompress(const char *input, size_t input_size, char *output, size_t max_output_size) {
	return LZ4_decompress_safe(input, output, static_cast<int>(input_size), static_cast<int>(max_output_size));
}