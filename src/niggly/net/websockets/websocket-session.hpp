
#pragma once

#include "niggly/utils.hpp"

#include "niggly/net/buffer.hpp"

namespace niggly::net::detail {
class Session;
}

namespace niggly::net {

// -------------------------------------------------------------------------------- WebsocketSession

/**
 * A session is a two-way connection between a client and server.
 * The client connects to the server (WebsocketServer), creating a new
 * WebsocketSession object, which serves at the external interface for sending
 * and receiving data.
 *
 * @see WebsocketServer::Config for where to set the factory method.
 * @note It is important that instances of WebsocketSession are freed when `on_close`
 *       is called.
 */
class WebsocketSession {
private:
  struct Pimpl;
  std::unique_ptr<Pimpl> pimpl_;
  friend class detail::Session; //! The internal websocket session

public:
  WebsocketSession();
  virtual ~WebsocketSession();

  /**
   * @brief A callback that is called when a connection receives a new message.
   * @note Must be threadsafe
   *
   * Parameters
   * + `data` and `size` is the payload of the message. It must be decoded immediately,
   *   because the underlying buffer will be reused.
   */
  virtual void on_receive(const void* data, std::size_t size) = 0;

  /**
   * @brief Should result in the resouce being freed
   * @note must be threadsafe
   */
  virtual void on_close(std::error_code ec) = 0;

  /**
   * @brief Send a message to the other end.
   * @todo We need a movable allocator-aware type to encapsulate a sequence of buffers
   */
  void send_message(WebsocketBufferType&& buffer);

  /**
   * @brief A `send_message` call is finished, and the buffer is being returned.
   * If not overriden, then the buffer will be deleted normally.
   */
  virtual void on_return_buffer(WebsocketBufferType&& buffer) {}
};
} // namespace niggly::net
