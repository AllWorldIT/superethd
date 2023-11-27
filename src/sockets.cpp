/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "common.hpp"
#include "threads.hpp"

/**
 * @brief Create a udp socket.
 *
 * @param tdata Thread data.
 * @return int -1 on failure, 0 on success.
 */
int create_udp_socket(struct ThreadData *tdata) {
	socklen_t addrlen = sizeof(struct sockaddr_in6);

	// Creating UDP datagram socket
	if ((tdata->udp_socket = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		perror("ERROR: Socket creation failed");
		return -1;
	}

	// Set the send buffer size (SO_SNDBUF)
	int buffer_size = tdata->l4mtu * SETH_BUFFER_COUNT;
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

/**
 * @brief Destroy a UDP socket.
 *
 * @param tdata Thread data.
 */
void destroy_udp_socket(struct ThreadData *tdata) { close(tdata->udp_socket); }
