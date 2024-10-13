#pragma once
#ifndef LOG_H
#define LOG_H

#include "spdlog/spdlog.h"

#define DEFAULT_LOGGER "superLogger"
#define INFO(...) if (spdlog::get(DEFAULT_LOGGER) != nullptr) {spdlog::get(DEFAULT_LOGGER)->info(__VA_ARGS__);}
#define WARN(...) if (spdlog::get(DEFAULT_LOGGER) != nullptr) {spdlog::get(DEFAULT_LOGGER)->warn(__VA_ARGS__);}
#define ERR(...) if (spdlog::get(DEFAULT_LOGGER) != nullptr) {spdlog::get(DEFAULT_LOGGER)->error(__VA_ARGS__);}

#endif //LOG_H


