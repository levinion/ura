#pragma once

#include <cstdio> // IWYU pragma: keep
#include <cstdlib> // IWYU pragma: keep
#include <expected>
#include <string>
#include <utility>

namespace ura {

enum class ErrorCode {
  InvalidArgument,
  NotFound,
  FailedPrecondition,
  IoError,
  Unsupported,
  ResourceExhausted,
  DeviceError,
  NotInitialized,
  ScriptError,
  Internal,
};

inline const char* to_string(ErrorCode code) {
  switch (code) {
    case ErrorCode::InvalidArgument:
      return "InvalidArgument";
    case ErrorCode::NotFound:
      return "NotFound";
    case ErrorCode::FailedPrecondition:
      return "FailedPrecondition";
    case ErrorCode::IoError:
      return "IoError";
    case ErrorCode::Unsupported:
      return "Unsupported";
    case ErrorCode::ResourceExhausted:
      return "ResourceExhausted";
    case ErrorCode::DeviceError:
      return "DeviceError";
    case ErrorCode::NotInitialized:
      return "NotInitialized";
    case ErrorCode::ScriptError:
      return "ScriptError";
    case ErrorCode::Internal:
      return "Internal";
  }
  return "Unknown";
}

struct Error {
  ErrorCode code = ErrorCode::Internal;
  std::string message;
};

template<typename T>
using StatusOr = std::expected<T, Error>;

using Status = StatusOr<void>;

inline Status Ok() {
  return {};
}

inline std::unexpected<Error> InvalidArgument(std::string message) {
  return std::unexpected(
    Error { ErrorCode::InvalidArgument, std::move(message) }
  );
}

inline std::unexpected<Error> NotFound(std::string message) {
  return std::unexpected(Error { ErrorCode::NotFound, std::move(message) });
}

inline std::unexpected<Error> FailedPrecondition(std::string message) {
  return std::unexpected(
    Error { ErrorCode::FailedPrecondition, std::move(message) }
  );
}

inline std::unexpected<Error> IoError(std::string message) {
  return std::unexpected(Error { ErrorCode::IoError, std::move(message) });
}

inline std::unexpected<Error> Unsupported(std::string message) {
  return std::unexpected(Error { ErrorCode::Unsupported, std::move(message) });
}

inline std::unexpected<Error> ResourceExhausted(std::string message) {
  return std::unexpected(
    Error { ErrorCode::ResourceExhausted, std::move(message) }
  );
}

inline std::unexpected<Error> DeviceError(std::string message) {
  return std::unexpected(Error { ErrorCode::DeviceError, std::move(message) });
}

inline std::unexpected<Error> NotInitialized(std::string message) {
  return std::unexpected(
    Error { ErrorCode::NotInitialized, std::move(message) }
  );
}

inline std::unexpected<Error> ScriptError(std::string message) {
  return std::unexpected(Error { ErrorCode::ScriptError, std::move(message) });
}

inline std::unexpected<Error> Internal(std::string message) {
  return std::unexpected(Error { ErrorCode::Internal, std::move(message) });
}

} // namespace ura

#define URA_DETAIL_CONCAT_INNER(x, y) x##y
#define URA_DETAIL_CONCAT(x, y) URA_DETAIL_CONCAT_INNER(x, y)

#define RETURN_IF_ERROR(expr) \
  do { \
    auto _ura_status = (expr); \
    if (!_ura_status) \
      return std::unexpected(_ura_status.error()); \
  } while (false)

#define URA_DETAIL_ASSIGN_OR_RETURN(lhs, rexpr, id) \
  auto URA_DETAIL_CONCAT(_ura_result_, id) = (rexpr); \
  if (!URA_DETAIL_CONCAT(_ura_result_, id)) \
    return std::unexpected(URA_DETAIL_CONCAT(_ura_result_, id).error()); \
  lhs = std::move(URA_DETAIL_CONCAT(_ura_result_, id).value())

#define ASSIGN_OR_RETURN(lhs, rexpr) \
  URA_DETAIL_ASSIGN_OR_RETURN(lhs, rexpr, __COUNTER__)

#define EXIT_IF_ERROR(expr) \
  do { \
    auto _ura_status = (expr); \
    if (!_ura_status) { \
      std::fprintf( \
        stderr, \
        "error: %s: %s\n", \
        ::ura::to_string(_ura_status.error().code), \
        _ura_status.error().message.c_str() \
      ); \
      std::exit(1); \
    } \
  } while (false)
