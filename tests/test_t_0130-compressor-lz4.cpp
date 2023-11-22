#include <catch2/catch.hpp>
#include "accl/compressor_lz4.hpp"

TEST_CASE("CompressorLZ4 compression and decompression", "[CompressorLZ4]") {
    // Sample data
    std::string input = "Hello, world!";

    // Create an instance of the CompressorLZ4 class
    accl::CompressorLZ4 compressor;

    // Compress the input data
    std::vector<uint8_t> compressed = compressor.compress(input);

    // Decompress the compressed data
    std::string decompressed = compressor.decompress(compressed);

    // Verify the correctness of the compression and decompression
    REQUIRE(decompressed == input);
}