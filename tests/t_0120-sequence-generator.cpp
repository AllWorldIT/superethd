/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "libtests/framework.hpp"

TEST_CASE("Check buffer usage", "[buffers]") {
	// We should get a wrap-around, each sequence is 11 long, we have 26 characters in the alphabet, so if we add 12 more we should
	// wrap to A again and end on B
	accl::SequenceDataGenerator seqdata = accl::SequenceDataGenerator(11 * 26 + 12);

	const std::string expected_result{
		"A0123456789B0123456789C0123456789D0123456789E0123456789F0123456789G0123456789H0123456789I0123456789J0123456789K0123456789L"
		"0123456789M0123456789N0123456789O0123456789P0123456789Q0123456789R0123456789S0123456789T0123456789U0123456789V0123456789W0"
		"123456789X0123456789Y0123456789Z0123456789A0123456789B"};

	std::cout << "SEQUENCE DATA: " << std::endl << seqdata.asString() << std::endl;
	std::cout << "EXPECTED DATA: " << std::endl << expected_result << std::endl;

	// Make sure the result is as expected
	REQUIRE(seqdata.asString() == expected_result);
}
