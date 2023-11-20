/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

void tunnel_tap_read_handler(void *arg);
void tunnel_encoder_handler(void *arg);
void tunnel_socket_write_handler(void *arg);

void tunnel_read_socket_handler(void *arg);
