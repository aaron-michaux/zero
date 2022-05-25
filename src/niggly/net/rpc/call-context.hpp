
#pragma once

#include "status.hpp"

#include <chrono>
#include <functional>
#include <atomic>
#include <cstdint>
#include <memory>

namespace niggly::net {

class RpcAgent;

class CallContext {
private:
  std::shared_ptr<RpcAgent> agent_;
  uint64_t request_id_{0};
  std::chrono::steady_clock::time_point deadline_;
  std::function<void(Status status)> completion_{};
  mutable std::mutex padlock_;
  bool is_cancelled_{false};
  bool has_finished_{false};

  void finish_call_locked_(Status status, std::function<bool(WebsocketBufferType&)> serializer);

public:
  CallContext(std::shared_ptr<RpcAgent> agent, uint64_t request_id,
              std::chrono::steady_clock deadline)
      : agent_{std::move(agent)}, request_id_{request_id}, deadline_{deadline} {}

  /**
   * @brief The deadline for the call.
   */
  std::chrono::steady_clock::time_point deadline() const { return deadline_; }

  /**
   * @brief Attempt to cancel the call; may not work, since it could race with `finishCall`.
   */
  void cancel();

  /**
   * @brief Return true iff the call has been cancelled.
   */
  bool is_cancelled() const;

  /**
   * @brief Return true iff the call has finished
   */
  bool has_finished() const;

  /**
   * @brief A completion function that is executed after the response is sent.
   */
  void set_completion(std::function<void(Status status)> thunk) { completion_ = std::move(thunk); }

  /**
   * @brief Calling this method sends the response to the wire.
   * @param status The status response to write.
   * @param serializer A callback that writes the response to a `WebsocketBufferType`
   */
  void finish_call(Status status, std::function<bool(WebsocketBufferType&)> serializer = nullptr);
};

} // namespace niggly::net
