/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <map>
#include <mutex>
#include <sstream>
#include <string>

namespace accl {

enum class LogLevel { DEBUGGING = 1, INFO = 2, NOTICE = 3, WARNING = 4, ERROR = 5 };

extern std::map<std::string, LogLevel> logLevelMap;

class Logger {

	private:
		LogLevel log_level;
		LogLevel log_level_default;
		bool log_time;

		std::mutex mutex_;

		std::string _getLogLevelString(LogLevel level) const;

		std::string _logLevelToString(const LogLevel level) const;

		void _log(const LogLevel level, const std::string file, const std::string func, const unsigned int line,
				  std::string logline);

	public:
		Logger();

		inline void setLogLevel(const LogLevel level);

		bool setLogLevel(const std::string level);

		inline LogLevel getLogLevel() const;

		inline std::string getLogLevelDefaultString() const;

		inline std::string getLogLevelString() const;

		template <typename... Args>
		void log(const LogLevel level, const std::string file, const std::string func, const unsigned int line, Args... args);
};

// Global logger instance
extern Logger logger;

/**
 * @brief Set log level.
 *
 * @param level Log level.
 */
inline void Logger::setLogLevel(const LogLevel level) { log_level = level; };

/**
 * @brief Get current log level.
 *
 * @return LogLevel Log level that we're set to.
 */
inline LogLevel Logger::getLogLevel() const { return log_level; };

/**
 * @brief Get log level default as a string in lowercase.
 *
 * @return std::string Log level as a string in lowercase.
 */
inline std::string Logger::getLogLevelDefaultString() const { return _getLogLevelString(log_level_default); }

/**
 * @brief Get log level as a string in lowercase.
 *
 * @return std::string Log level as a string in lowercase.
 */
inline std::string Logger::getLogLevelString() const { return _getLogLevelString(log_level); }

/**
 * @brief Log something.
 *
 * @tparam Args Variable number of arguments.
 * @param level Log level.
 * @param file File name that the log method is called from.
 * @param func Function or method that the log method is called from.
 * @param line Line number from where the log method is called.
 * @param args Arguments comprising of items to be concatenated to log.
 */
template <typename... Args>
void Logger::log(const LogLevel level, const std::string file, const std::string func, const unsigned int line, Args... args) {
	if (level >= log_level) {
		std::ostringstream stream;
		(stream << ... << args);
		_log(level, file, func, line, stream.str());
	}
}

} // namespace accl

// The LOG_DEBUG_INTERNAL will be factored out of the code if compiled without debugging
#ifdef DEBUG
#define LOG_DEBUG_INTERNAL(...) accl::logger.log(accl::LogLevel::DEBUGGING, __FILE__, __func__, __LINE__, __VA_ARGS__);
#else
#define LOG_DEBUG_INTERNAL(...) ((void)0);
#endif

// Helper macro's for all the types of logging
#define LOG_DEBUG(...) accl::logger.log(accl::LogLevel::DEBUGGING, __FILE__, __func__, __LINE__, __VA_ARGS__);
#define LOG_INFO(...) accl::logger.log(accl::LogLevel::INFO, __FILE__, __func__, __LINE__, __VA_ARGS__);
#define LOG_NOTICE(...) accl::logger.log(accl::LogLevel::NOTICE, __FILE__, __func__, __LINE__, __VA_ARGS__);
#define LOG_WARNING(...) accl::logger.log(accl::LogLevel::WARNING, __FILE__, __func__, __LINE__, __VA_ARGS__);
#define LOG_ERROR(...) accl::logger.log(accl::LogLevel::ERROR, __FILE__, __func__, __LINE__, __VA_ARGS__);

#ifdef UNIT_TESTING
#define UT_ASSERT(...) assert(__VA_ARGS__)
#else
#define UT_ASSERT(...) ((void)0)
#endif