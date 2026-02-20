#pragma once

#include <absl/strings/ascii.h>
#include <fmt/base.h>
#include <libnotify/notify.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <utility>
#include "ura/ura.hpp" // IWYU pragma: keep

namespace ura::log {

class UraLogLevel {
public:
  enum class Level { Silent, Critical, Error, Warn, Info, Debug, Trace };

  inline static UraLogLevel from_str(std::string _level) {
    auto log_level = UraLogLevel {};
    auto level_str = absl::AsciiStrToLower(_level);
    if (level_str == "debug")
      log_level.level = Level::Debug;
    else if (level_str == "error")
      log_level.level = Level::Error;
    else if (level_str == "warn")
      log_level.level = Level::Warn;
    else if (level_str == "info")
      log_level.level = Level::Info;
    else if (level_str == "trace")
      log_level.level = Level::Trace;
    else if (level_str == "critical")
      log_level.level = Level::Critical;
    else if (level_str == "silent")
      log_level.level = Level::Silent;
    else
      log_level.level = Level::Info;
    return log_level;
  }

  inline spdlog::level::level_enum as_spd_level() {
    switch (this->level) {
      case Level::Silent:
        return spdlog::level::off;
      case Level::Critical:
        return spdlog::level::critical;
      case Level::Error:
        return spdlog::level::err;
      case Level::Warn:
        return spdlog::level::warn;
      case Level::Info:
        return spdlog::level::info;
      case Level::Debug:
        return spdlog::level::debug;
      case Level::Trace:
        return spdlog::level::trace;
    }
    std::unreachable();
  }

  inline wlr_log_importance as_wlr_level() {
    switch (level) {
      case Level::Silent:
        return WLR_SILENT;
      case Level::Error:
        return WLR_ERROR;
      case Level::Info:
        return WLR_INFO;
      case Level::Debug:
        return WLR_DEBUG;
      case Level::Critical:
        return WLR_ERROR;
      case Level::Warn:
        return WLR_INFO;
      case Level::Trace:
        return WLR_DEBUG;
    }
    std::unreachable();
  }

private:
  Level level = Level::Info;
};

void init();
void destroy();

inline void notify(const std::string& summary, const std::string& body) {
  auto notification = notify_notification_new(summary.c_str(), body.c_str(), 0);
  auto _ = notify_notification_show(notification, 0);
}

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
