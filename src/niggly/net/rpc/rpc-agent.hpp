
#pragma once

#include "call-headers.hpp"
#include "call-context.hpp"

#include "niggly/net/buffer.hpp"
#include "niggly/net/websockets/websocket-session.hpp"

//#include <boost/asio/steady_timer.hpp>

#include <cstdint>
#include <memory>

namespace niggly::net {

/**
 * @brief An `RpcAgent` can serve and send RPC requests.
 */
template <typename Executor, typename SteadyTimerType>
class RpcAgent : public WebsocketSession,
                 public std::enable_shared_from_this<RpcAgent<Executor, SteadyTimerType>> {
public:
  using ThunkType = std::function<void()>;

  using CallContextType = CallContext<Executor, SteadyTimerType>;

  /**
   * @brief Creates a thunk to handle a call represented by the passed `context` object
   * NOTE: The memory pointed to by `data` and `size` may (will) evaporate immediately,
   *       and thus must be deserialized/copied before the thunk is returned.
   * NOTE: `context` holds a pointer back to this object.
   */
  using CallHandler = std::function<ThunkType(std::shared_ptr<CallContextType> context,
                                              const void* data, std::size_t size)>;
  using CompletionHandler = std::function<void(Status status, const void* data, std::size_t size)>;

  using SteadyTimerFactory = std::function<SteadyTimerType()>;

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
   * @param handler The RPC call handler for incoming requests.
   * @param timer_factory Creates timers for managing request timeouts.
   */
  RpcAgent(Executor executor, CallHandler handler, SteadyTimerFactory timer_factory)
      : executor_{executor}, handler_{std::move(handler)}, timer_factory_{
                                                               std::move(timer_factory)} {}

  /**
   * @brief An rpc client sends a type-erased rpc call.
   * @param call_id The call to make.
   * @param deadline_millis Number of milliseconds to wait for a response.
   * @param serializer A thunk that serializes the request data.
   * @param completion The completion handler to execute with the response.
   */
  void perform_rpc_call(uint32_t call_id, uint32_t deadline_millis,
                        std::function<bool(BufferType&)> serializer, CompletionHandler completion);

private:
  /**
   * @brief Safely close resources
   */
  void on_close(uint16_t code_code, std::string_view reason) override;

  /**
   * @brief Method that receives calls from a server agent.
   */
  void on_receive(const void* data, std::size_t size) override;

  void handle_request_(const void* data, std::size_t size);
  void handle_response_(const void* data, std::size_t size);
  void finish_response_(const detail::ResponseEnvelopeHeader& header);
};

} // namespace niggly::net

#include "impl/rpc-agent_impl.hpp"
