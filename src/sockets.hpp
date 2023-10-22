/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "threads.hpp"

int create_udp_socket(struct ThreadData *tdata);
int destroy_udp_socket(struct ThreadData *tdata);
