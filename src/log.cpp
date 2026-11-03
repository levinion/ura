#include "ura/core/log.hpp"

#include <libnotify/notification.h>
#include <libnotify/notify.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/syslog_sink.h>
#include <spdlog/spdlog.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <unistd.h>

#include "ura/util/string.hpp"

extern "C" {
#include <wlr/util/log.h>
}

namespace ura {

namespace {

  class UraLogLevel {
  public:
    enum class Level { Silent, Critical, Error, Warn, Info, Debug, Trace };

    static UraLogLevel from_str(std::string value) {
      auto level = UraLogLevel {};
      auto name = util::ascii_lower(value);
      if (name == "debug")
        level.level = Level::Debug;
      else if (name == "error")
        level.level = Level::Error;
      else if (name == "warn")
        level.level = Level::Warn;
      else if (name == "info")
        level.level = Level::Info;
      else if (name == "trace")
        level.level = Level::Trace;
      else if (name == "critical")
        level.level = Level::Critical;
      else if (name == "silent")
        level.level = Level::Silent;
      else
        level.level = Level::Info;
      return level;
    }

    spdlog::level::level_enum as_spd_level() const {
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

    wlr_log_importance as_wlr_level() const {
      switch (this->level) {
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

  spdlog::level::level_enum
  wlr_log_to_spdlog_level(wlr_log_importance importance) {
    switch (importance) {
      case WLR_ERROR:
        return spdlog::level::err;
      case WLR_INFO:
        return spdlog::level::info;
      case WLR_DEBUG:
        return spdlog::level::debug;
      default:
        return spdlog::level::trace;
    }
  }

  void wlr_logger_wrapper(
    wlr_log_importance importance,
    const char* fmt,
    va_list args
  ) {
    char message[10240];
    vsnprintf(message, sizeof(message), fmt, args);
    spdlog::log(wlr_log_to_spdlog_level(importance), message);
  }

} // namespace

std::unique_ptr<UraLogger> UraLogger::init() {
  auto logger = std::make_unique<UraLogger>();

  auto level_str = std::getenv("URA_LOG");
  auto level = UraLogLevel::from_str(level_str ? level_str : "INFO");

  wlr_log_init(level.as_wlr_level(), wlr_logger_wrapper);

  auto syslog_sink = std::make_shared<spdlog::sinks::syslog_sink_mt>(
    "ura",
    LOG_PID,
    LOG_USER,
    true
  );
  auto ansicolor_sink =
    std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();

  logger->spdlog_logger = std::make_shared<spdlog::logger>(
    "ura",
    spdlog::sinks_init_list { ansicolor_sink, syslog_sink }
  );

  logger->spdlog_logger->set_error_handler([](const std::string& err_msg) {
    // do nothing here
  });

  spdlog::set_default_logger(logger->spdlog_logger);
  spdlog::set_level(level.as_spd_level());

  notify_init("ura");

  return logger;
}

void UraLogger::destroy() {
  wlr_log_init(WLR_INFO, nullptr);
  notify_uninit();
}

} // namespace ura

namespace ura::log {

void notify(const std::string& summary, const std::string& body) {
  auto notification = notify_notification_new(summary.c_str(), body.c_str(), 0);
  auto _ = notify_notification_show(notification, 0);
}

} // namespace ura::log
