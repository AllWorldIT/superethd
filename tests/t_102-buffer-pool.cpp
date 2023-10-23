/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "libtests/framework.hpp"


TEST_CASE("Check buffers and buffer pools work", "[buffers]") {
	accl::BufferPool buffer_pool = accl::BufferPool(1024, 10);

	assert(buffer_pool.getBufferCount() == 10);
}


TEST_CASE("Check retrieving a buffer for use", "[buffers]") {
	accl::BufferPool buffer_pool = accl::BufferPool(1024, 10);

	assert(buffer_pool.getBufferCount() == 10);

	auto buffer = buffer_pool.pop();

}