
#pragma once

#include "call-headers.hpp"

#include <cstdint>
#include <memory>

namespace niggly::net {

class RpcAgent : public WebsocketSession, public std::enable_shared_from_this<RpcAgent> {
public:
  using thunk_type = std::function<void()>;

  /**
   * @brief Create a thunk for the passed call context.
   * NOTE: the handler must treat `data` and `size` like volatile memory.
   * NOTE: `context` holds a pointer back to this object.
   */
  using CallHandler = std::function<thunk_type(std::shared_ptr<CallContext> context,
                                               const void* data, std::size_t size)>;

private:
  std::vector<CallHandler> handlers_;

public:
  RpcAgent(std::vector<CallHandler> handlers) : handlers_{std::move(handlers)} {}

  void on_receive(const void* data, std::size_t size) override {
    detail::RequestEnvelopeHeader header;
    if (!decode(data, size, header))
      return; // Corrupt data

    if (header.call_id >= handlers_.size()) {
      // Immediately send an error! Unhandled rpc call code!
      return;
    }

    // Setup the call
    auto context = std::make_shared<CallContext>(shared_from_this(), header);

    // Execute the call
    const auto& handler = handlers_[header.call_id];
    executor.execute(handler(std::move(context), header.data, header.size));
  }

  void on_close(std::error_code ec) override {
    // how to safely free all resources (!)
  }
};

} // namespace niggly::net
