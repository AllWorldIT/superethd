/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "StreamCompressor.hpp"

extern "C" {
#include <lz4.h>
}

class CompressorLZ4 : public StreamCompressor {
	public:
		int compress(const char *input, size_t input_size, char *output, size_t max_output_size) override;

		int decompress(const char *input, size_t input_size, char *output, size_t max_output_size) override ;
};