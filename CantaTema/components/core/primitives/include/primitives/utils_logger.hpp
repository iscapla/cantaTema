#ifndef __UTILS_LOGGER_HPP
#define __UTILS_LOGGER_HPP

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <spdlog/spdlog.h>

extern spdlog::logger *logger;

/**
 * @brief Initialize logger.
 * The project will use 2 loggers handlers: one to STD console and other saved daily on a file under 'log' folder
 * 
 */
void util_logger_init(void);

/**
 * @brief Initialize logger only for test purposes.
 * The project will no prompt any message
 * 
 */
void util_logger_init_for_test(void);

#endif //__UTILS_LOGGER_HPP