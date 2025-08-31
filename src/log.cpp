#include "ura/core/log.hpp"
#include <libnotify/notification.h>
#include <libnotify/notify.h>
#include <spdlog/spdlog.h>
#include "spdlog/sinks/syslog_sink.h"

namespace ura::log {

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
  char message[1024];
  vsnprintf(message, sizeof(message), fmt, args);
  spdlog::log(wlr_log_to_spdlog_level(importance), message);
}

void init() {
  auto level_str = std::getenv("URA_LOG");
  auto level = UraLogLevel::from_str(level_str ? level_str : "INFO");

  wlr_log_init(level.as_wlr_level(), wlr_logger_wrapper);

  auto syslog_logger = std::make_shared<spdlog::sinks::syslog_sink_mt>(
    "ura",
    LOG_PID,
    LOG_USER,
    true
  );
  spdlog::default_logger()->sinks().push_back(syslog_logger);
  spdlog::set_level(level.as_spd_level());

  notify_init("ura");
}

void destroy() {
  notify_uninit();
}

} // namespace ura::log
