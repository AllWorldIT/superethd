/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

void tunnel_read_tap_handler(void *arg);
void tunnel_encoding_handler(void *arg);
void tunnel_write_socket_handler(void *arg);
/*
void *tunnel_read_socket_handler(void *arg);

void *tunnel_write_tap_handler(void *arg);
*/