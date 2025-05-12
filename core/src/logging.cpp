#include "logging.hpp"

#include <spdlog/spdlog.h>

#include "exception.hpp"

namespace ailoy {

void set_log_level(const std::string &level) {
  if (level == "debug")
    spdlog::set_level(spdlog::level::debug);
  else if (level == "info")
    spdlog::set_level(spdlog::level::info);
  else if (level == "warn")
    spdlog::set_level(spdlog::level::warn);
  else if (level == "error")
    spdlog::set_level(spdlog::level::err);
  else if (level == "critical")
    spdlog::set_level(spdlog::level::critical);
  else
    throw exception("Unknown log level: " + level);
}

void set_log_format(const std::string &fmt) { spdlog::set_pattern(fmt); }

void debug(const std::string &msg) { spdlog::debug(msg); }

void info(const std::string &msg) { spdlog::info(msg); }

void warn(const std::string &msg) { spdlog::warn(msg); }

void error(const std::string &msg) { spdlog::error(msg); }

void critical(const std::string &msg) { spdlog::critical(msg); }

} // namespace ailoy
