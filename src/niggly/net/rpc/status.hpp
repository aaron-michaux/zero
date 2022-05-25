
#pragma once

#include <string>
#include <string_view>

namespace niggly::net {

enum class StatusCode : int8_t {
  OK,
  CANCELLED,
  UNKNOWN,
  INVALID_ARGUMENT,
  DEADLINE_EXCEEDED,
  NOT_FOUND,
  ALREADY_EXISTS,
  PERMISSION_DENIED,
  UNAUTHENTICATED,
  RESOURCE_EXHAUSTED,
  FAILED_PRECONDITION,
  ABORTED,
  OUT_OF_RANGE,
  UNIMPLEMENTED,
  INTERNAL,
  UNAVAILABLE,
  DATA_LOSS,
  DO_NOT_USE
};

class Status {
private:
  std::string error_message_{};
  std::string error_details_{};
  StatusCode status_code_{StatusCode::OK};

public:
  Status() = default;
  Status(StatusCode status_code, std::string error_message)
      : status_code_{status_code}, error_message_{std::move(error_message)} {}
  Status(StatusCode status_code, std::string error_message, std::string error_details)
      : status_code_{status_code}, error_message_{std::move(error_message)},
        error_details_{std::move(error_details)} {}

  StatusCode error_code() const { return status_code_; }
  std::string_view error_message() const { return error_message_; }
  std::string_view error_details() const { return error_details_; }
  bool ok() const { return status_code_ == StatusCode::OK; }
};

} // namespace niggly::net
