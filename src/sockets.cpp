/*
 * Socket handling.
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

extern "C" {
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
}

#include "threads.hpp"
#include "util.hpp"

int create_udp_socket(struct ThreadData *tdata) {
	socklen_t addrlen = sizeof(struct sockaddr_in6);

	// Creating UDP datagram socket
	if ((tdata->udp_socket = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		perror("ERROR: Socket creation failed");
		return -1;
	}

	// Set the send buffer size (SO_SNDBUF)
	int buffer_size = 1024 * 1024 * 64;
	if (setsockopt(tdata->udp_socket, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size)) < 0) {
		perror("Error setting send buffer size");
		close(tdata->udp_socket);
		return -1;
	}
	if (setsockopt(tdata->udp_socket, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) < 0) {
		perror("Error setting receive buffer size");
		close(tdata->udp_socket);
		return -1;
	}

	// Bind UDP socket source address
	if (bind(tdata->udp_socket, (struct sockaddr *)&tdata->local_addr, addrlen) == -1) {
		perror("ERROR: Failed to bind source address to socket\n");
		close(tdata->udp_socket);
		return -1;
	}

	return 0;
}

void destroy_udp_socket(struct ThreadData *tdata) { close(tdata->udp_socket); }
