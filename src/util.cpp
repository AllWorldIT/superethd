/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "util.hpp"
#include "exceptions.hpp"
#include "libaccl/logger.hpp"
#include <arpa/inet.h>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <sys/socket.h>

/**
 * @brief Convert a uint8_t array into a char buffer.
 *
 * @param array uint8_t array to convert.
 * @param length Length of array.
 * @return char* char buffer.
 */
char *uint8_array_to_char_buffer(const uint8_t *array, size_t length) {
	// Allocate memory for the buffer (+1 for the null terminator)
	char *buffer = (char *)malloc(length + 1);
	if (!buffer) {
		return NULL; // Memory allocation failed
	}

	// Copy the values
	for (size_t i = 0; i < length; i++) {
		buffer[i] = (char)array[i];
	}

	// Null-terminate the buffer
	buffer[length] = '\0';

	return buffer;
}

/**
 * @brief Convert a string to an sockaddr_storage
 *
 * @param str String to convert into a sockaddr_storage structure.
 * @param port Port number to set.
 * @throw SuperEthernetTunnelConfigurationException If the conversion fails.
 */
std::shared_ptr<struct sockaddr_storage> to_sockaddr_storage(const std::string &str, uint16_t port) {
	std::shared_ptr<struct sockaddr_storage> result = std::make_shared<struct sockaddr_storage>();

	// Try converting as IPv6 address
	sockaddr_in6 addr6;
	memset(&addr6, 0, sizeof(addr6));
	if (inet_pton(AF_INET6, str.data(), &addr6.sin6_addr) > 0) {
		addr6.sin6_family = AF_INET6;
		if (port) {
			addr6.sin6_port = htons(port);
		}
		memcpy(result.get(), &addr6, sizeof(addr6));
		return result;
	}

	// Try converting as IPv4 address
	sockaddr_in addr4;
	if (inet_pton(AF_INET, str.data(), &addr4.sin_addr) > 0) {
		addr4.sin_family = AF_INET;
		if (port) {
			addr4.sin_port = htons(port);
		}
		memcpy(result.get(), &addr4, sizeof(addr4));
		return result;
	}

	// Conversion failed
	throw SuperEthernetTunnelConfigException("Invalid IP address: " + std::string(str));
}

/**
 * @brief Return true if IPv4 or false if IPv6
 *
 * @param addr IPv6 address to check.
 * @return bool Returns `true` on IPv4 and `false` on IPv6.
 */

bool is_ipv4(const std::shared_ptr<sockaddr_storage> addr) {

	if (addr->ss_family == AF_INET) {
		// It's IPv4
		return 0; // Not an IPv4-mapped IPv6 address
	} else if (addr->ss_family == AF_INET6) {
		// It's IPv6, now check if it's an IPv4-mapped IPv6 address
		const struct sockaddr_in6 *addr6 = reinterpret_cast<const struct sockaddr_in6 *>(addr.get());
		if (addr6->sin6_addr.s6_addr[10] == 0xFF && addr6->sin6_addr.s6_addr[11] == 0xFF) {
			return 1;
		}

		return 0; // Not an IPv4-mapped IPv6 address

	} else {
		// Unsupported address family
		return -1; // Error or unsupported type
	}
}

/**
 * @brief Detect if the packet sequence is wrapping.
 *
 * @param cur Current sequence.
 * @param prev Previous sequence.
 * @return int 1 if wrapping, 0 if not.
 */
int is_sequence_wrapping(uint32_t cur, uint32_t prev) {
	// Check if the current sequence number is less than the previous one
	// while accounting for wrapping
	return cur < prev && (prev - cur) > (UINT32_MAX / 2);
}

/**
 * @brief Get the string representation of an IP.
 *
 * @param addr sockaddr_storage structure.
 * @return std::string
 */
std::string get_ipstr(const sockaddr_storage *addr) {
	if (addr->ss_family == AF_INET) {
		sockaddr_in *addr_in = (sockaddr_in *)addr;
		return get_ipv4_str(addr_in);
	} else if (addr->ss_family == AF_INET6) {
		sockaddr_in6 *addr_in6 = (sockaddr_in6 *)addr;
		return get_ipv6_str(addr_in6);
	} else {
		return "Unknown";
	}
}

/**
 * @brief Get the ipv4 str object.
 *
 * @param addr
 * @return std::string
 */
std::string get_ipv4_str(const sockaddr_in *addr) {
	// Get string from address
	char ipv4_str[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &(addr->sin_addr), ipv4_str, sizeof(ipv4_str)) == NULL) {
		throw SuperEthernetTunnelRuntimeException("Cannot convert address to string");
	}
	return std::string(ipv4_str);
}

/**
 * @brief Get the ipv6 str object from a sockaddr_in6 structure.
 *
 * @param addr6
 * @return std::string
 */
std::string get_ipv6_str(const sockaddr_in6 *addr6) {
	// Get string from address
	char ipv6_str[INET6_ADDRSTRLEN];
	if (inet_ntop(AF_INET6, &(addr6->sin6_addr), ipv6_str, sizeof(ipv6_str)) == NULL) {
		throw SuperEthernetTunnelRuntimeException("Cannot convert address to string");
	}
	return std::string(ipv6_str);
}

/**
 * @brief Split a string by delimiters.
 *
 * @param input Input string.
 * @param delimiters Delimiters to split by.
 * @return std::vector<std::string>
 */
std::vector<std::string> splitByDelimiters(const std::string &input, const std::string &delimiters) {
	// Create a regex based on delimiters
	std::regex regexPattern("[" + delimiters + "]+");
	std::sregex_token_iterator iter(input.begin(), input.end(), regexPattern, -1);
	std::sregex_token_iterator end;

	std::vector<std::string> result(iter, end);

	// Remove possible empty strings at the end
	result.erase(std::remove_if(result.begin(), result.end(), [](const std::string &s) { return s.empty(); }), result.end());

	return result;
}

/**
 * @brief Get the key from a sockaddr_storage structure.
 *
 * @param addr sockaddr_storage structure.
 * @return std::array<uint8_t, 16> Key.
 */
std::array<uint8_t, 16> get_key_from_sockaddr(sockaddr_storage *addr) {
	std::array<uint8_t, 16> key;

	// Check if it's an IPv4 or IPv6 address
	if (addr->ss_family == AF_INET) {
		sockaddr_in *addr_in = (sockaddr_in *)addr;
		memset(key.data(), 0, 10); // Zero out the first 10 bytes (IPv4-mapped IPv6 address prefix)
		key[10] = 0xFF;
		key[11] = 0xFF;
		memcpy(key.data() + 12, &addr_in->sin_addr, 4);

	} else if (addr->ss_family == AF_INET6) {
		sockaddr_in6 *addr_in6 = (sockaddr_in6 *)addr;
		memcpy(key.data(), &addr_in6->sin6_addr, 16);
	}

	return key;
}

void dumpSockaddr(const sockaddr *sa) {
	if (sa->sa_family == AF_INET) { // IPv4
		char ip[INET_ADDRSTRLEN] = {0};
		sockaddr_in *addr_in = (sockaddr_in *)sa;

		inet_ntop(AF_INET, &(addr_in->sin_addr), ip, INET_ADDRSTRLEN);
		LOG_DEBUG("IPv4 Address: ", ip, ", Port: ", ntohs(addr_in->sin_port));

	} else if (sa->sa_family == AF_INET6) { // IPv6
		char ip[INET6_ADDRSTRLEN] = {0};
		sockaddr_in6 *addr_in6 = (sockaddr_in6 *)sa;

		inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ip, INET6_ADDRSTRLEN);
		LOG_DEBUG("IPv6 Address: ", ip, ", Port: ", ntohs(addr_in6->sin6_port));

	} else {
		std::cerr << "Unknown AF family" << std::endl;
	}
}

/**
 * @brief Convert a sockaddr_storage structure to an IPv6 sockaddr_storage structure.
 *
 * @param addr sockaddr_storage structure.
 * @return std::shared_ptr<struct sockaddr_storage>
 */
std::shared_ptr<struct sockaddr_storage> to_sockaddr_storage_ipv6(const std::shared_ptr<sockaddr_storage> addr) {
	std::shared_ptr<struct sockaddr_storage> result = std::make_shared<struct sockaddr_storage>();

	if (addr->ss_family == AF_INET6) {
		memcpy(result.get(), addr.get(), sizeof(struct sockaddr_storage));

	} else if (addr->ss_family == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)addr.get();
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)result.get();

		memset(&addr6->sin6_addr.s6_addr[0], 0, 10);
		memset(&addr6->sin6_addr.s6_addr[10], 0xff, 2);
		// Set family, port, flowinfo, scope_id
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = addr4->sin_port;
		addr6->sin6_flowinfo = 0;
		addr6->sin6_scope_id = 0;
		// Copy in IPv4 address portion
		memcpy(&addr6->sin6_addr.s6_addr[12], &addr4->sin_addr.s_addr, 4);
	}

	return result;
}
