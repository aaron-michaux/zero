

#pragma once

#include "call-headers.hpp"
#include "call-context.hpp"

#include "niggly/net/websocket-session.hpp"

#include <boost/asio/steady_timer.hpp>

#include <cstdint>
#include <memory>

namespace niggly::net {

template <typename Executor>
void RpcAgent<Executor>::perform_rpc_call(uint32_t call_id, uint32_t deadline_millis,
                                          std::function<bool(WebsocketBufferType&)> serializer,
                                          CompletionHandler completion) {
  WebsocketBufferType buffer;
  const auto request_id = next_request_id_.fetch_add(1, std::memory_order_acq_rel);

  // Serialize the response header
  if (!detail::encode_request_header(buffer, request_id, call_id, deadline_millis)) {
    completion(Status{StatusCode::ABORTED}, nullptr, 0);
    return;
  }

  // Serialize the response parameters
  if (serializer && !serializer(buffer)) {
    completion(Status{StatusCode::ABORTED}, nullptr, 0);
    return;
  }

  RpcResponse response;
  response.completion = std::move(completion);

  if (deadline_millis > 0) { // Setup the timeout
    response.timer = timer_factory_();
    response.timer.expires_after(std::chrono::milliseconds{deadline_millis});
    response.timer.async_wait(
        [weak = this->weak_from_this(), request_id](const boost::system::error_code& ec) {
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

template <typename Executor> void RpcAgent<Executor>::on_close(std::error_code ec) {}

template <typename Executor>
void RpcAgent<Executor>::on_receive(const void* data, std::size_t size) {
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

template <typename Executor>
void RpcAgent<Executor>::handle_request_(const void* data, std::size_t size) {
  detail::RequestEnvelopeHeader header;

  if (!detail::decode(header, data, size)) {
    return; // Corrupt data
  }

  assert(header.is_request);

  // Calculate the deadline
  const auto millis = std::chrono::milliseconds{header.deadline_millis};
  const auto deadline = (millis.count() != 0) ? std::chrono::steady_clock::now() + millis
                                              : std::chrono::steady_clock::time_point::max();

  // Create the call context
  auto context =
      std::make_shared<CallContext>(this->shared_from_this(), header.request_id, deadline);

  // Execute the call
  executor_.execute(handler_(std::move(context), header.call_id, header.data, header.size));
}

template <typename Executor>
void RpcAgent<Executor>::handle_response_(const void* data, std::size_t size) {
  detail::ResponseEnvelopeHeader header;
  if (!detail::decode(header, data, size)) {
    return; // Corrupt data
  }
  finish_reponse_(std::move(header));
}

template <typename Executor>
void RpcAgent<Executor>::finish_reponse_(detail::ResponseEnvelopeHeader header) {
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
      response.completion(std::move(header.status), header.data, header.size); // run it
  }
}

} // namespace niggly::net
