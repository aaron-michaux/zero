
#pragma once

#include "call-headers.hpp"
#include "call-context.hpp"

#include "niggly/net/websocket-session.hpp"

#include <boost/asio/steady_timer.hpp>

#include <cstdint>
#include <memory>

namespace niggly::net {

template <typename Executor>
class RpcAgent : public WebsocketSession, public std::enable_shared_from_this<RpcAgent<Executor>> {
public:
  using ThunkType = std::function<void()>;

  /**
   * @brief Creates a thunk to handle a call represented by the passed `context` object
   * NOTE: the handler must treat `data` and `size` like volatile memory.
   * NOTE: `context` holds a pointer back to this object.
   */
  using CallHandler =
      std::function<ThunkType(std::shared_ptr<CallContext<Executor>> context, uint32_t call_id,
                              const void* data, std::size_t size)>;
  using CompletionHandler = std::function<void(Status status, const void* data, std::size_t size)>;

  using SteadyTimerFactory = std::function<boost::asio::steady_timer()>;

private:
  Executor executor_;
  CallHandler handler_;
  SteadyTimerFactory timer_factory_;

  std::atomic<uint64_t> next_request_id_{1};

  struct RpcResponse {
    CompletionHandler completion_handler;
    boost::asio::steady_timer timeout;
  };

  // NOTE: in theory we could use a memory address as the call-id,
  //       but then some crafty person will send fake data to
  //       crash the server. Maybe there's a better way, but for
  //       now I think this global map is required.
  mutable std::mutex padlock_;
  std::unordered_map<uint64_t, RpcResponse> outstanding_calls_;

public:
  /**
   * @brief Set up a new agent for serving rpc calls.
   * @param executor The executor for handling incoming requests.
   * @param timer_factory Creates timers for managing request timeouts.
   * @param handlers The RPC call handlers that are matched to incoming requests.
   */
  RpcAgent(Executor executor, CallHandler handler, SteadyTimerFactory timer_factory)
      : executor_{executor}, handler_{std::move(handler)}, timer_factory_{
                                                               std::move(timer_factory)} {}

  /**
   * @brief A type-erased rpc call.
   * @param call_id The call to make.
   * @param deadline_millis Number of milliseconds to wait for a response.
   * @param serializer A thunk that serializes the request data.
   * @param completion The completion handler to execute with the response.
   */
  void perform_rpc_call(uint32_t call_id, uint32_t deadline_millis,
                        std::function<bool(WebsocketBufferType&)> serializer,
                        CompletionHandler completion);
  /**
   * @brief Safely close resources
   */
  void on_close(std::error_code ec) override;

  /**
   * @brief Method that receives calls from a server agent.
   */
  void on_receive(const void* data, std::size_t size) override;

private:
  void handle_request_(const void* data, std::size_t size);
  void handle_response_(const void* data, std::size_t size);
  void finish_reponse_(detail::ResponseEnvelopeHeader header);
};

} // namespace niggly::net

#include "rpc-agent_impl.hpp"
