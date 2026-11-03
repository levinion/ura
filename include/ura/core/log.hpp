#pragma once

#include <fmt/base.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <string>
#include <utility>

namespace ura {

class UraLogger {
public:
  static std::unique_ptr<UraLogger> init();
  void destroy();

  ~UraLogger() {
    this->destroy();
  }

  std::shared_ptr<spdlog::logger> spdlog_logger;
};

} // namespace ura

namespace ura::log {

void notify(const std::string& summary, const std::string& body);

template<typename... Args>
void error(const fmt::format_string<Args...>& fmt, Args&&... args) {
  spdlog::error(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void warn(const fmt::format_string<Args...>& fmt, Args&&... args) {
  spdlog::warn(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void debug(const fmt::format_string<Args...>& fmt, Args&&... args) {
  spdlog::debug(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void info(const fmt::format_string<Args...>& fmt, Args&&... args) {
  spdlog::info(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void trace(const fmt::format_string<Args...>& fmt, Args&&... args) {
  spdlog::trace(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void critical(const fmt::format_string<Args...>& fmt, Args&&... args) {
  spdlog::critical(fmt, std::forward<Args>(args)...);
}

} // namespace ura::log
