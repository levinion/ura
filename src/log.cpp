#include "ura/core/log.hpp"
#include <libnotify/notification.h>
#include <libnotify/notify.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/spdlog.h>
#include <memory>
#include "spdlog/sinks/syslog_sink.h"

namespace ura {

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

std::unique_ptr<UraLogger> UraLogger::init() {
  auto logger = std::make_unique<UraLogger>();

  auto level_str = std::getenv("URA_LOG");
  logger->level = UraLogLevel::from_str(level_str ? level_str : "INFO");

  wlr_log_init(logger->level.as_wlr_level(), wlr_logger_wrapper);

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
  spdlog::set_default_logger(logger->spdlog_logger);
  spdlog::set_level(logger->level.as_spd_level());

  notify_init("ura");

  return logger;
}

void UraLogger::destroy() {
  wlr_log_init(this->level.as_wlr_level(), nullptr);
  notify_uninit();
}
} // namespace ura
