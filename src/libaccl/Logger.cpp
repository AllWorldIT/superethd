/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "Logger.hpp"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>

namespace accl {

std::string Logger::_getLogLevelString(const LogLevel level) const {
	std::string level_lc(_logLevelToString(level));
	std::transform(level_lc.begin(), level_lc.end(), level_lc.begin(), [](unsigned char c) { return std::tolower(c); });

	return level_lc;
}

std::string Logger::_logLevelToString(const LogLevel level) const {
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

/**
 * @brief Set logging level.
 *
 * @param level Log level in string form.
 * @return true If the log level was set successfully.
 * @return false If the log level was not set due to an invalid value.
 */
bool Logger::setLogLevel(const std::string level) {
	std::string level_lc(level);
	std::transform(level_lc.begin(), level_lc.end(), level_lc.begin(), [](unsigned char c) { return std::tolower(c); });

	if (accl::logLevelMap.find(level_lc) != accl::logLevelMap.end()) {
		log_level = logLevelMap[level_lc];
		return true;
	}

	return false;
}

void Logger::_log(const LogLevel level, const std::string file, const std::string func, const unsigned int line,
				  std::string logline) {
	if (level >= log_level) {
		std::ostringstream stream;

		// Grab current time
		auto now_utc = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm *utc_tm = std::gmtime(&now_utc);

		// Output time and log level
		stream << std::put_time(utc_tm, "%Y-%m-%d %H:%M:%S") << " [" << _logLevelToString(level) << "] ";
		// If we're debugging output the func, file and line
		if (log_level == LogLevel::DEBUGGING) {
			stream << "(" << func << ":" << file << ":" << line << ") ";
		}
		stream << logline;
		// Log mutex
		std::lock_guard<std::mutex> lock(mutex_);
		std::cerr << stream.str() << std::endl;
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