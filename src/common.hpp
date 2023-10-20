
/*
 * Common header.
 * Copyright (C) 2023, AllWorldIT.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

// Make the packed attribute easier to use
#define __seth_packed __attribute__((packed))

// Default tunnel name
#define SETH_DEFAULT_TUNNEL_NAME "seth0"

// Default buffer count
#define SETH_BUFFER_COUNT 5000
// Number of messages to get at maximum from recvmm
#define SETH_MAX_RECVMM_MESSAGES 1000

// Minimum transmission packet size
#define SETH_MIN_TXSIZE 1200
// Maximum transmission packet size
#define SETH_MAX_TXSIZE 9000

// Minimum device MTU size
#define SETH_MIN_MTU_SIZE 1200
// Maximum device MTU size
#define SETH_MAX_MTU_SIZE 9198


/*
 * Other useful constants
 */

#define SETH_MIN_ETHERNET_FRAME_SIZE 64

