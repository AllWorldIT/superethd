/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "Logger.hpp"
#include <algorithm>

namespace accl {

std::string Logger::_getLogLevelString(LogLevel level) const {
	std::string level_lc(_logLevelToString(level));
	std::transform(level_lc.begin(), level_lc.end(), level_lc.begin(), [](unsigned char c) { return std::tolower(c); });

	return level_lc;
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

Logger::Logger() {
#if defined(DEBUG) || defined(UT_TESTING)
	log_level_default = LogLevel::DEBUGGING;
#else
	log_level_default = LogLevel::NOTICE;
#endif
	log_level = log_level_default;
}

void Logger::setLogLevel(LogLevel level) { log_level = level; }

bool Logger::setLogLevel(const std::string level) {
	std::string level_lc(level);
	std::transform(level_lc.begin(), level_lc.end(), level_lc.begin(), [](unsigned char c) { return std::tolower(c); });

	if (accl::logLevelMap.find(level_lc) != accl::logLevelMap.end()) {
		log_level = logLevelMap[level_lc];
		return true;
	}

	return false;
}

LogLevel Logger::getLogLevel() { return log_level; }

std::string Logger::getLogLevelDefaultString() const { return _getLogLevelString(log_level_default); }

std::string Logger::getLogLevelString() const { return _getLogLevelString(log_level); }

void Logger::log(LogLevel level, const std::string &message) {
	std::lock_guard<std::mutex> lock(mutex_);
	if (level >= log_level) {
		std::cerr << "[" << _logLevelToString(level) << "] " << message << std::endl;
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