
#include "stdinc.hpp"

#include "call-context.hpp"

#include "rpc-agent.hpp"

namespace niggly::net {

void CallContext::cancel() {
  std::lock_guard lock{padlock_};
  is_cancelled_ = true;
  finiah_call_locked_({StatusCode::CANCELLED}, nullptr);
}

bool CallContext::is_cancelled() const {
  std::lock_guard lock{padlock_};
  return is_cancelled_;
}

bool CallContext::has_finished() const {
  std::lock_guard lock{padlock_};
  return has_finished_;
}

void CallContext::finish_call(Status status, std::function<bool(WebsocketBufferType&)> serializer) {
  std::lock_guard lock{padlock_};
  finish_call_locked_(std::move(status), std::move(serializeer));
}

void CallContext::finish_call_locked_(Status status,
                                      std::function<bool(WebsocketBufferType&)> serializer) {
  // Check if this has already been done
  if (has_finished_)
    return;
  has_finished_ = true;

  WebsocketBufferType buffer;
  buffer.reserve(512);

  auto set_and_send_error = [this, &buffer](StatusCode code) {
    Status status{};
    buffer.clear();
    encode_response_header(buffer, request_id_, status);
    agent_->send_message(std::move(buffer));
    if (completion_) {
      completion_(std::move(status));
    }
  };

  const auto deadline_exceeded = deadline_ < std::chrono::steady_clock::now();
  if (deadline_exceeded) {
    set_and_send_error(StatusCode::DEADLINE_EXCEEDED);
    return;
  }

  if (!encode_response_header(buffer, request_id_, status)) {
    set_and_send_error(StatusCode::DATA_LOSS);
    return;
  }

  if (status.ok() && serializer && !serializer(buffer)) {
    set_and_send_error(StatusCode::DATA_LOSS);
    return;
  }

  agent_->send_message(std::move(buffer));
  if (completion_) {
    completion_(std::move(status));
  }
}

} // namespace niggly::net
