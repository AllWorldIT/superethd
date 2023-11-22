/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "libtests/framework.hpp"
#include <stdexcept>
#include <string>

#include "libaccl/StreamCompressorZSTD.hpp"

TEST_CASE("StreamCompressorZSTD Compression", "[compressor]") {
	accl::StreamCompressorZSTD compressor;

	SECTION("Compression of empty input") {
		const std::string input = "";
		const size_t max_output_size = 100;
		char output[max_output_size];

		REQUIRE(compressor.compress(input.data(), input.size(), output, max_output_size) == 9);
	}

	SECTION("Compression of non-empty input") {
		const std::string input = "Hello, World!";
		const size_t max_output_size = 100;
		char output[max_output_size];

		REQUIRE(compressor.compress(input.data(), input.size(), output, max_output_size) > 0);
	}
}

TEST_CASE("StreamCompressorZSTD Decompression", "[StreamCompressorZSTD]") {
	accl::StreamCompressorZSTD compressor;

	SECTION("Decompression of valid input") {
		const std::string input = "compressed data";
		const size_t max_output_size = 100;
		char output[max_output_size];

		// Compress the input data
		const int compressed_size = compressor.compress(input.data(), input.size(), output, max_output_size);

		REQUIRE(compressed_size > 0);

		// Decompress the compressed data
		const int decompressed_size = compressor.decompress(output, compressed_size, output, max_output_size);

		REQUIRE(decompressed_size == 15);

		REQUIRE(std::string(output, decompressed_size) == input);
	}

	SECTION("Decompression of invalid input") {
		const std::string input = "invalid compressed data";
		const size_t max_output_size = 100;
		char output[max_output_size];

		// Decompress the invalid compressed data
		const int decompressed_size = compressor.decompress(input.data(), input.size(), output, max_output_size);

		REQUIRE(decompressed_size < 0);
	}
}

TEST_CASE("StreamCompressorZSTD Error Handling", "[StreamCompressorZSTD]") {
	accl::StreamCompressorZSTD compressor;

	SECTION("Compression error") {
		const std::string input = "input data";
		const size_t max_output_size = 1;
		char output[max_output_size];

		// Attempt to compress the input data with insufficient output buffer size
		const int compressed_size = compressor.compress(input.data(), input.size(), output, max_output_size);

		REQUIRE(compressed_size == -70);
	}

	SECTION("Decompression error") {
		const std::string input = "compressed data";
		const size_t max_output_size = 100;
		char output[max_output_size];

		// Attempt to decompress the compressed data with insufficient output buffer size
		const int decompressed_zie = compressor.decompress(input.data(), input.size(), output, max_output_size);

		REQUIRE(decompressed_zie == -10);
	}
}