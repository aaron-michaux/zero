
#pragma once

#include "websocket-session.hpp"

#include "niggly/net/buffer.hpp"

#include "niggly/utils.hpp"

namespace boost::asio {
class io_context;
}

namespace niggly::net {

class WebsocketClient {
private:
  struct Pimpl;
  std::unique_ptr<Pimpl> pimpl_;

public:
  /**
   * Exceptions
   * + std::bad_alloc
   */
  WebsocketClient(boost::asio::io_context& io_context);
  WebsocketClient(const WebsocketClient&) = delete;
  WebsocketClient(WebsocketClient&&) = default;
  virtual ~WebsocketClient();
  WebsocketClient& operator=(const WebsocketClient&) = delete;
  WebsocketClient& operator=(WebsocketClient&&) = default;

  /**
   * Exceptions
   * + std::runtime_error Already connected
   */
  void connect(std::string_view url);

  void send_message(BufferType&& buffer);

  /**
   * @brief Should result in the resouce being freed
   * @note must be threadsafe
   */
  virtual void on_close(std::error_code ec) {}

  virtual void on_error(std::error_code ec) {}

  /**
   * @brief A `send_message` call is finished, and the buffer is being returned.
   * If not overriden, then the buffer will be deleted normally.
   */
  virtual void on_return_buffer(BufferType&& buffer) {}
};

} // namespace niggly::net
