/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <vector>

int read_hex_dump_into_buffer(const char *hex_dump, uint8_t **buffer, size_t *length);
char *uint8_array_to_char_buffer(const uint8_t *array, size_t length);

char *create_sequence_data(size_t length);

std::shared_ptr<struct sockaddr_storage> to_sockaddr_storage(const std::string &str, uint16_t port);

/**
 * @brief Convert a string to an sockaddr_storage
 *
 * @param str String to convert into a sockaddr_storage structure.
 * @throw SuperEthernetTunnelConfigurationException If the conversion fails.
 */
inline std::shared_ptr<struct sockaddr_storage> to_sockaddr_storage(const std::string &str) { return to_sockaddr_storage(str, 0); };

bool is_ipv4(const std::shared_ptr<sockaddr_storage> addr);

int is_sequence_wrapping(uint32_t cur, uint32_t prev);

std::string get_ipstr(const sockaddr_storage *addr);
std::string get_ipv4_str(const sockaddr_in *addr);
std::string get_ipv6_str(const sockaddr_in6 *addr6);

std::vector<std::string> splitByDelimiters(const std::string &input, const std::string &delimiters);

std::array<uint8_t, 16> get_key_from_sockaddr(sockaddr_storage *sockaddr);

void dumpSockaddr(const sockaddr *sa);

std::shared_ptr<struct sockaddr_storage> to_sockaddr_storage_ipv6(const std::shared_ptr<sockaddr_storage> addr);
