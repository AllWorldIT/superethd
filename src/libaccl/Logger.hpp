/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <iostream>
#include <map>
#include <mutex>
#include <string>

namespace accl {

enum class LogLevel { DEBUGGING = 1, INFO = 2, NOTICE = 3, WARNING = 4, ERROR = 5 };

extern std::map<std::string, LogLevel> logLevelMap;

class Logger {

	private:
		LogLevel log_level;
		LogLevel log_level_default;

		std::mutex mutex_;

		std::string _getLogLevelString(LogLevel level) const;

		std::string _logLevelToString(LogLevel level) const;



			public : Logger();

		void setLogLevel(LogLevel level);

		bool setLogLevel(const std::string level) ;

		LogLevel getLogLevel();

		std::string getLogLevelDefaultString() const ;
		
		std::string getLogLevelString() const;

		void log(LogLevel level, const std::string &message);
};

// Global logger instance
extern Logger logger;

// The LOG_DEBUG_INTERNAL will be factored out of the code if compiled without debugging
#ifdef DEBUG
#define LOG_DEBUG_INTERNAL(fmt, ...)                                                                                               \
	accl::logger.log(accl::LogLevel::DEBUGGING, std::format("({}:{}:{}): " fmt, __FILE__, __func__, __LINE__, ##__VA_ARGS__));
#else
#define LOG_DEBUG_INTERNAL(fmt, ...) ((void)0);
#endif

// Helper macro's for all the types of logging
#define LOG_DEBUG(fmt, ...) accl::logger.log(accl::LogLevel::DEBUGGING, std::format(fmt, ##__VA_ARGS__));
#define LOG_INFO(fmt, ...) accl::logger.log(accl::LogLevel::INFO, std::format(fmt, ##__VA_ARGS__));
#define LOG_NOTICE(fmt, ...) accl::logger.log(accl::LogLevel::NOTICE, std::format(fmt, ##__VA_ARGS__));
#define LOG_WARNING(fmt, ...) accl::logger.log(accl::LogLevel::WARNING, std::format(fmt, ##__VA_ARGS__));
#define LOG_ERROR(fmt, ...) accl::logger.log(accl::LogLevel::ERROR, std::format(fmt, ##__VA_ARGS__));

// Helper macro's for printing out to the standard handles
#define CERR(fmt, ...) std::cerr << std::format(fmt, ##__VA_ARGS__) << std::endl;
#define COUT(fmt, ...) std::cout << std::format(fmt, ##__VA_ARGS__) << std::endl;

#ifdef UNIT_TESTING
#define UT_ASSERT(...) assert(__VA_ARGS__)
#else
#define UT_ASSERT(...) ((void)0)
#endif

} // namespace accl