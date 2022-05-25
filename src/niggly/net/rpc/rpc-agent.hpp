
#pragma once

#include "call-headers.hpp"
#include "call-context.hpp"

#include "niggly/net/websocket-session.hpp"

#include <cstdint>
#include <memory>

namespace niggly::net {

template <typename Executor>
class RpcAgent final : protected WebsocketSession, public std::enable_shared_from_this<RpcAgent> {
public:
  using ThunkType = std::function<void()>;

  /**
   * @brief Create a thunk for the passed call context.
   * NOTE: the handler must treat `data` and `size` like volatile memory.
   * NOTE: `context` holds a pointer back to this object.
   */
  using CallHandler = std::function<ThunkType(std::shared_ptr<CallContext> context,
                                              const void* data, std::size_t size)>;
  using CompletionHandler = std::function<void(Status status, const void* data, std::size_t size)>;

  using SteadyTimerFactory = std::function<boost::asio::steady_timer()>;

private:
  Executor executor_;
  SteadyTimerFactory timer_factory_;
  std::vector<CallHandler> handlers_;

  std::atomic<uint64_t> next_request_id_{1};

  struct RpcResponse {
    CompletionHandler completion_handler;
    boost::asio::steady_timer timeout;
  };

  mutable std::mutex padlock_;
  std::unordered_map<uint64_t, RpcResponse> outstanding_calls_;

public:
  /**
   * @brief Set up a new agent for serving rpc calls.
   * @param executor The executor for handling incoming requests.
   * @param timer_factory Creates timers for managing request timeouts.
   * @param handlers The RPC call handlers that are matched to incoming requests.
   */
  RpcAgent(Executor executor, SteadyTimerFactory timer_factory, std::vector<CallHandler> handlers)
      : executor_{executor}, timer_factory_{std::move(timer_factory)}, handlers_{
                                                                           std::move(handlers)} {}

  /**
   * @brief A type-erased rpc call.
   * @param call_id The call to make.
   * @param deadline_millis Number of milliseconds to wait for a response.
   * @param serializer A thunk that serializes the request data.
   * @param completion The completion handler to execute with the response.
   */
  void perform_rpc_call(uint32_t call_id, uint32_t deadline_millis,
                        std::function<bool(WebsocketBufferType&)> serializer,
                        CompletionHandler completion) {
    WebsocketBufferType buffer;
    const auto request_id = next_request_id_.fetch_add(1, std::memory_order_acq_rel);

    // Serialize the response header
    if (!encode_request_header(buffer, request_id, call_id, deadline_millis)) {
      completion({StatusCode::ABORTED}, nullptr, 0);
      return;
    }

    // Serialize the response parameters
    if (serializer && !serilizer(buffer)) {
      completion({StatusCode::ABORTED}, nullptr, 0);
      return;
    }

    RpcResponse response;
    response.completion = std::move(completion);

    if (deadline_millis > 0) { // Setup the timeout
      response.timer = timer_factory_();
      response.timer.expires_after(std::chrono::milliseconds{deadline_millis});
      response.timer.async_wait(
          [weak = weak_from_this(), request_id](const boost::system::error_code& ec) {
            if (!ec) {
              auto ptr = weak.lock();
              if (ptr != nullptr) {
                detail::ResponseEnvelopeHeader header;
                header.request_id = request_id;
                header.status = Status{StatusCode::DEADLINE_EXCEEDED};
                ptr->finish_response_(std::move(header));
              }
            }
          });
    }

    { // Prepare to record the response
      std::lock_guard lock{padlock_};
      outstanding_calls_.insert({request_id, std::move(response)});
    }

    // Send the message to the wire
    send_message(std::move(buffer));
  }

protected:
  /**
   * @brief Safely close resources
   */
  void on_close(std::error_code ec) override {}

  /**
   * @brief Method that receives calls from a server agent.
   */
  void on_receive(const void* data, std::size_t size) override {
    if (size == 0) {
      return; // There's no header: corrupt data
    }

    const bool is_request = static_cast<const char*>(data)[0] != 0;

    if (is_request) { // Receiving a request is the server side of an RPC call
      handle_request_(data, size);
    } else { // Receiving a response is the end part of the client side of an RPC call
      handle_response_(data, size);
    }
  }

private:
  void handle_request_(const void* data, std::size_t size) {
    detail::RequestEnvelopeHeader header;

    if (!decode(data, size, header)) {
      return; // Corrupt data
    }

    assert(header.is_request);

    if (header.call_id >= handlers_.size()) {
      // Immediately send an error! Unhandled rpc call code!
      return;
    }

    // Calculate the deadline
    const auto millis = std::chrono::milliseconds{header.deadline_millis};
    const auto deadline = (millis.count() != 0) ? std::chrono::steady_clock::now() + millis
                                                : std::chrono::steady_clock::time_point::max();

    // Create the call context
    auto context = std::make_shared<CallContext>(shared_from_this(), request_id, deadline);

    // Execute the call
    const auto& handler = handlers_[header.call_id];
    executor_.execute(handler(std::move(context), header.data, header.size));
  }

  void handle_response_(const void* data, std::size_t size) {
    detail::ResponseEnvelopeHeader header;
    if (!decode(data, size, header)) {
      return; // Corrupt data
    }
    finish_reponse_(std::move(header));
  }

  void finish_reponse_(detail::ResponseEnvelopeHeader header) {
    assert(!header.is_request);

    bool retreived = false;
    RpcResponse response;
    { // Grab the completion handler, if it still exists
      std::lock_guard lock{padlock_};
      auto ii = outstanding_calls_.find(header.request_id);
      if (ii != cend(outstanding_calls_)) {
        retreived = true;
        response = std::move(ii->second);
        outstanding_calls_.erase(ii);
      }
    }

    if (retreived) {           // If the rpc-reponse has not yet been handled
      response.timer.cancel(); // cancel any timeout
      if (response.completion) // and if there's a completion
        completion(std::move(header.status), header.data, header.size); // run it
    }
  }
};

} // namespace niggly::net
