/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <stdexcept>
#include <string>

/*
 * @brief Packet not currently supported exception.
 *
 */

class PacketNotSupportedEception : public std::runtime_error {
	public:
		PacketNotSupportedEception(const std::string &message) : std::runtime_error(message) {}
};

/*
 * @brief Packet malformed exception.
 *
 */
class PacketMalformedEception : public std::runtime_error {
	public:
		PacketMalformedEception(const std::string &message) : std::runtime_error(message) {}
};

/*
 * @brief Invalid packet operation exception.
 *
 */
class PacketOperationInvalid : public std::runtime_error {
	public:
		PacketOperationInvalid(const std::string &message) : std::runtime_error(message) {}
};
