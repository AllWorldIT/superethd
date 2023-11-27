/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

void tunnel_tap_read_handler(void *arg);
void tunnel_encoder_handler(void *arg);
void tunnel_socket_write_handler(void *arg);

void tunnel_socket_read_handler(void *arg);
void tunnel_decoder_handler(void *arg);
void tunnel_tap_write_handler(void *arg);
