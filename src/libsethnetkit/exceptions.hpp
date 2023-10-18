/*
 * Exceptions.
 * Copyright (C) 2023, AllWorldIT.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
