
#include <iostream>

#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "primitives/tool_paths.hpp"
#include "primitives/utils_logger.hpp"

spdlog::logger *logger{nullptr};

void util_logger_init(void)
{

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    console_sink->set_pattern("[%d/%m/%C %T.%e] [%^%8!l%$] [%t] %v");

    std::filesystem::path logs_path = ToolPath::get_path_for_logs() / "daily.txt";
    auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(logs_path.string(), 0, 0);
    file_sink->set_level(spdlog::level::debug);
    file_sink->set_pattern("[%d/%m/%C %T.%e] [%l] [%t] %v");

    logger = new spdlog::logger("logger", {console_sink, file_sink});
    logger->set_level(spdlog::level::trace);

    logger->trace("Logger initialized");
}

void util_logger_init_for_test(void)
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::err);
    console_sink->set_pattern("[%d/%m/%C %T.%e] [%^%8!l%$] [%t] %v");

    logger = new spdlog::logger("logger", {console_sink});
    logger->set_level(spdlog::level::critical);
}