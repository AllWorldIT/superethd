/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "Logger.hpp"

namespace accl {

Logger::Logger() : log_level(LogLevel::NOTICE) {}

void Logger::setLevel(LogLevel level) { log_level = level; }
LogLevel Logger::getLevel() { return log_level; }

void Logger::log(LogLevel level, const std::string &message) {
	std::lock_guard<std::mutex> lock(mutex_);
	if (level >= log_level) {
		std::cerr << "[" << toString(level) << "] " << message << std::endl;
	}
}

std::string Logger::toString(LogLevel level) {
	switch (level) {
	case LogLevel::DEBUGGING:
		return "DEBUG";
	case LogLevel::INFO:
		return "INFO";
	case LogLevel::WARNING:
		return "WARNING";
	case LogLevel::ERROR:
		return "ERROR";
	default:
		return "UNKNOWN";
	}
}

// Define the global logger instance
Logger logger;


} // namespace accl