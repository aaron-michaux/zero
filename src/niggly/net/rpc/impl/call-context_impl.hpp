
#pragma once

namespace niggly::net {

template <typename Extecutor>
std::chrono::steady_clock::time_point CallContext<Extecutor>::deadline() const {
  return deadline_;
}

template <typename Extecutor> void CallContext<Extecutor>::cancel() {
  std::lock_guard lock{padlock_};
  is_cancelled_ = true;
  finish_call_locked_({StatusCode::CANCELLED}, nullptr);
}

template <typename Extecutor> bool CallContext<Extecutor>::is_cancelled() const {
  std::lock_guard lock{padlock_};
  return is_cancelled_;
}

template <typename Extecutor> bool CallContext<Extecutor>::has_finished() const {
  std::lock_guard lock{padlock_};
  return has_finished_;
}

template <typename Extecutor>
void CallContext<Extecutor>::set_completion(std::function<void(Status status)> thunk) {
  completion_ = std::move(thunk);
}

template <typename Extecutor>
void CallContext<Extecutor>::finish_call(Status status,
                                         std::function<bool(WebsocketBufferType&)> serializer) {
  std::lock_guard lock{padlock_};
  finish_call_locked_(std::move(status), std::move(serializer));
}

// ----------------------------------------------------------------------------- finish_call_locked_

template <typename Extecutor>
void CallContext<Extecutor>::finish_call_locked_(
    Status status, std::function<bool(WebsocketBufferType&)> serializer) {
  // Check if this has already been done
  if (has_finished_)
    return;
  has_finished_ = true;

  WebsocketBufferType buffer;
  buffer.reserve(512);

  auto set_and_send_error = [this, &buffer](StatusCode code) {
    Status status{};
    buffer.clear();
    detail::encode_response_header(buffer, request_id_, status);
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

  if (!detail::encode_response_header(buffer, request_id_, status)) {
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
