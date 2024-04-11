/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <stdexcept>
#include <string>

class SuperEthernetTunnelException : public std::runtime_error {
	public:
		SuperEthernetTunnelException(const std::string &message) : std::runtime_error(message) {}
};

class SuperEthernetTunnelRuntimeException : public SuperEthernetTunnelException {
	public:
		SuperEthernetTunnelRuntimeException(const std::string &message) : SuperEthernetTunnelException(message) {}
};

class SuperEthernetTunnelConfigException : public SuperEthernetTunnelException {
	public:
		SuperEthernetTunnelConfigException(const std::string &message) : SuperEthernetTunnelException(message) {}
};
