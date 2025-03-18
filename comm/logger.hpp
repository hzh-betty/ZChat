#pragma once

/*
    对spdlog进行二次封装
*/
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>
#include <iostream>
#include <memory>

namespace zchat
{
    // mode - 运行模式： true-发布模式； false调试模式
    std::shared_ptr<spdlog::logger> g_default_logger;
    void init_logger(bool mode, const std::string &filename, int32_t level)
    {
        // 1.调试模式，将所有日志都输出
        if (mode == false)
        {
            g_default_logger = spdlog::stdout_color_mt("default-logger");
            g_default_logger->set_level(spdlog::level::level_enum::trace);
            g_default_logger->flush_on(spdlog::level::level_enum::trace);
        }
        // 2.发布模式，将日志输出到指定文件
        else
        {
            g_default_logger = spdlog::basic_logger_mt("default-logger", filename);
            g_default_logger->set_level(static_cast<spdlog::level::level_enum>(level));
            g_default_logger->flush_on(static_cast<spdlog::level::level_enum>(level));
        }
        g_default_logger->set_pattern("[%n][%H:%M:%S][%t][%-8l]%v");
    }
};

#define LOG_TRACE(format, ...) zchat::g_default_logger->trace(std::string("[{}:{}] ") + format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) zchat::g_default_logger->debug(std::string("[{}:{}] ") + format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(format, ...) zchat::g_default_logger->info(std::string("[{}:{}] ") + format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(format, ...) zchat::g_default_logger->warn(std::string("[{}:{}] ") + format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) zchat::g_default_logger->error(std::string("[{}:{}] ") + format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_FATAL(format, ...) zchat::g_default_logger->critical(std::string("[{}:{}] ") + format, __FILE__, __LINE__, ##__VA_ARGS__)