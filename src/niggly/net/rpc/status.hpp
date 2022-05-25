
#pragma once

#include <string>
#include <string_view>

namespace niggly::net {

enum class StatusCode : int8_t {
  OK = 0,
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
  Status(StatusCode status_code = StatusCode::OK, std::string error_message = "",
         std::string error_details = "")
      : error_message_{std::move(error_message)}, error_details_{std::move(error_details)},
        status_code_{status_code} {}

  StatusCode error_code() const { return status_code_; }
  std::string_view error_message() const { return error_message_; }
  std::string_view error_details() const { return error_details_; }
  bool ok() const { return status_code_ == StatusCode::OK; }

  bool operator==(const Status& o) const {
    return (status_code_ == o.status_code_) && (error_message_ == o.error_message_) &&
           (error_details_ == o.error_details_);
  }
  bool operator!=(const Status& o) const { return !(*this == o); }
};

} // namespace niggly::net
