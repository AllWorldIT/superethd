/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "threads.hpp"

int create_udp_socket(struct ThreadData *tdata);
void destroy_udp_socket(struct ThreadData *tdata);
