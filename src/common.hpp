/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
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

