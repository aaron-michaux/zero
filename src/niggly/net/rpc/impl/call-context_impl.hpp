
#pragma once

namespace niggly::net {

template <typename Extecutor, typename SteadyTimerType>
std::chrono::steady_clock::time_point CallContext<Extecutor, SteadyTimerType>::deadline() const {
  return deadline_;
}

template <typename Extecutor, typename SteadyTimerType>
void CallContext<Extecutor, SteadyTimerType>::cancel() {
  std::lock_guard lock{padlock_};
  is_cancelled_ = true;
  // TODO: stop token...
  finish_call_locked_({StatusCode::CANCELLED}, nullptr);
}

template <typename Extecutor, typename SteadyTimerType>
bool CallContext<Extecutor, SteadyTimerType>::is_cancelled() const {
  std::lock_guard lock{padlock_};
  return is_cancelled_;
}

template <typename Extecutor, typename SteadyTimerType>
bool CallContext<Extecutor, SteadyTimerType>::has_finished() const {
  std::lock_guard lock{padlock_};
  return has_finished_;
}

template <typename Extecutor, typename SteadyTimerType>
void CallContext<Extecutor, SteadyTimerType>::set_completion(
    std::function<void(Status status)> thunk) {
  std::lock_guard lock{padlock_};
  completion_ = std::move(thunk);
}

template <typename Extecutor, typename SteadyTimerType>
void CallContext<Extecutor, SteadyTimerType>::finish_call(
    Status status, std::function<bool(BufferType&)> serializer) {
  std::lock_guard lock{padlock_};
  finish_call_locked_(std::move(status), std::move(serializer));
}

// ----------------------------------------------------------------------------- finish_call_locked_

template <typename Extecutor, typename SteadyTimerType>
void CallContext<Extecutor, SteadyTimerType>::finish_call_locked_(
    Status status, std::function<bool(BufferType&)> serializer) {
  // Check if this has already been done
  if (has_finished_)
    return;
  has_finished_ = true;

  BufferType buffer;
  buffer.reserve(512);

  auto set_and_send_error = [this, &buffer](StatusCode code) {
    Status status{code};
    buffer.clear();
    detail::encode_response_header(buffer, request_id_, status);
    agent_->send_message(std::move(buffer));
    if (completion_) {
      completion_(std::move(status));
    }
  };

  const auto deadline_exceeded = (deadline_ < std::chrono::steady_clock::now());
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

template <typename Extecutor, typename SteadyTimerType>
string CallContext<Extecutor, SteadyTimerType>::to_string() const {
  const std::chrono::system_clock::time_point deadline =
      std::chrono::system_clock::now() + (deadline_ - std::chrono::steady_clock::now());
  return format("CallContext(id={}, call-id={}, is-cancelled={}, has-finished={}, deadline={})",
                request_id_, call_id_, is_cancelled_, has_finished_,
                Timestamp{deadline}.to_string());
}

} // namespace niggly::net
