
#pragma once

#include "niggly/utils.hpp"

#include "niggly/net/buffer.hpp"

namespace niggly::net::detail {
class Session;
}

namespace niggly::net {

enum class WebsocketOperation : int {
  HANDSHAKE, // websocket handshake
  ACCEPT,    // Accepting a new connection
  READ,      // During read operation
  WRITE      // During a write operation
};

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
   * @brief Connect to a websocket endpoint.
   * Returns an error if already connected to an endpoint.
   */
  std::error_code connect(std::string_view host, uint16_t port);

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
  virtual void on_close() {}

  /**
   * @brief Non-write errors result in the session being closed
   * @note must be threadsafe
   */
  virtual void on_error(WebsocketOperation operation, std::error_code ec) {
    INFO("error on op={}: {}", int(operation), ec.message());
  }

  /**
   * @brief Send a message to the other end.
   * @todo We need a movable allocator-aware type to encapsulate a sequence of buffers
   */
  void send_message(BufferType&& buffer);

  /**
   * @brief A `send_message` call is finished, and the buffer is being returned.
   * If not overriden, then the buffer will be deleted normally.
   */
  virtual void on_return_buffer(BufferType&& buffer) {}
};

} // namespace niggly::net
