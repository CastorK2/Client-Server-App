#include "Logger.hpp"
#include "Log.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <memory>

Logger::Logger(){
    initialized = false;
}

void Logger::initializeServer(){
    if (!initialized){
        initialized = true;
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_level(spdlog::level::warn);

        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/log.txt", true);
        fileSink->set_level(spdlog::level::info);

        std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};
        auto logger = std::make_shared<spdlog::logger>(DEFAULT_LOGGER, sinks.begin(), sinks.end());
        logger->flush_on(spdlog::level::trace);
        logger->set_pattern("%^[%H:%M:%S] [Server] [%l] %v");
        spdlog::register_logger(logger);
    }else{
        INFO("Logger already initialized");
    }
}

void Logger::initializeClient(){
    if (!initialized){
        initialized = true;
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_level(spdlog::level::warn);

        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/log.txt", true);
        fileSink->set_level(spdlog::level::info);

        std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};
        auto logger = std::make_shared<spdlog::logger>(DEFAULT_LOGGER, sinks.begin(), sinks.end());
        logger->flush_on(spdlog::level::trace);
        logger->set_pattern("%^[%H:%M:%S] [Client] [%l] %v");
        spdlog::register_logger(logger);
    }else{
        INFO("Logger already initialized");
    }
}