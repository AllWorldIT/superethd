/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "Logger.hpp"

namespace accl {

Logger::Logger() : log_level(LogLevel::NOTICE) {}

void Logger::setLogLevel(LogLevel level) { log_level = level; }
LogLevel Logger::getLogLevel() { return log_level; }
std::string Logger::getLogLevelString() const { return _logLevelToString(log_level); }

void Logger::log(LogLevel level, const std::string &message) {
	std::lock_guard<std::mutex> lock(mutex_);
	if (level >= log_level) {
		std::cerr << "[" << _logLevelToString(level) << "] " << message << std::endl;
	}
}

std::string Logger::_logLevelToString(LogLevel level) const {
	switch (level) {
	case LogLevel::DEBUGGING:
		return "DEBUG";
	case LogLevel::INFO:
		return "INFO";
	case LogLevel::NOTICE:
		return "NOTICE";
	case LogLevel::WARNING:
		return "WARNING";
	case LogLevel::ERROR:
		return "ERROR";
	default:
		return "UNKNOWN";
	}
}

std::map<std::string, LogLevel> logLevelMap = {{"debug", LogLevel::DEBUGGING},
											   {"info", LogLevel::INFO},
											   {"notice", LogLevel::NOTICE},
											   {"warn", LogLevel::WARNING},
											   {"error", LogLevel::ERROR}};

// Define the global logger instance
Logger logger;

} // namespace accl