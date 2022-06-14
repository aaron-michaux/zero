
#pragma once

#include "status.hpp"

#include "niggly/net/buffer.hpp"

#include <chrono>
#include <functional>
#include <atomic>
#include <cstdint>
#include <memory>

namespace niggly::net {

template <typename Executor> class RpcAgent;

/**
 * @brief Context for a single active RPC call.
 *
 * This context exists on the server side of the RPC call, while it is being executed. There is no
 * equivalent client side context object.
 */
template <typename Executor> class CallContext {
private:
  std::shared_ptr<RpcAgent<Executor>> agent_;
  uint64_t request_id_{0};
  std::chrono::steady_clock::time_point deadline_;
  std::function<void(Status status)> completion_{};
  mutable std::mutex padlock_;
  uint32_t call_id_{0};
  bool is_cancelled_{false};
  bool has_finished_{false};

  void finish_call_locked_(Status status, std::function<bool(BufferType&)> serializer);

public:
  CallContext(std::shared_ptr<RpcAgent<Executor>> agent, uint64_t request_id, uint32_t call_id,
              std::chrono::steady_clock::time_point deadline)
      : agent_{std::move(agent)}, request_id_{request_id}, deadline_{deadline}, call_id_{call_id} {}

  /**
   * @brief The request-id... may be useful for idempotent semantics
   */
  uint64_t request_id() const { return request_id_; }

  /**
   * @brief The call-id... ie., which rpc function is being called, `do_this`, or `do_that`.
   */
  uint32_t call_id() const { return call_id_; }

  /**
   * @brief The deadline for the call.
   */
  std::chrono::steady_clock::time_point deadline() const;

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
  void set_completion(std::function<void(Status status)> thunk);

  /**
   * @brief Calling this method sends the response to the wire.
   * @param status The status response to write.
   * @param serializer A callback that writes the response to a `BufferType`
   */
  void finish_call(Status status, std::function<bool(BufferType&)> serializer = nullptr);
};

} // namespace niggly::net

#include "impl/call-context_impl.hpp"
