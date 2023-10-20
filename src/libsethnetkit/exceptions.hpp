/*
 * Exceptions.
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
