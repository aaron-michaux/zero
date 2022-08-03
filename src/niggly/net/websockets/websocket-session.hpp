
#pragma once

#include "niggly/utils.hpp"

#include "niggly/net/buffer.hpp"

namespace boost::asio {
class io_context;
}

namespace niggly::net::detail {
class Session;
}

namespace niggly::net {

enum class WebsocketOperation : int {
  CONNECT,   // A (client) is initiating a connection
  HANDSHAKE, // websocket handshake
  ACCEPT,    // Accepting a new connection
  READ,      // During read operation
  WRITE,     // During a write operation
  CLOSE      // The websocket stream is being closed
};

constexpr std::string_view str(WebsocketOperation op) {
#define CASE(x)                                                                                    \
  case WebsocketOperation::x:                                                                      \
    return #x
  switch (op) {
    CASE(CONNECT);
    CASE(HANDSHAKE);
    CASE(ACCEPT);
    CASE(READ);
    CASE(WRITE);
    CASE(CLOSE);
  }
#undef CASE
  return "<unknown case>";
}

// -------------------------------------------------------------------------------- WebsocketSession

/**
 * A `WebsocketSession` is a two-way connection between a client and server.
 *
 * When a server receives a new connection, a `WebsocketSession` instance
 * is created to manage two-way communication with the client -- i.e, sending and
 * receiving.
 *
 * `WebsocketSession` can also be used to create a client connection to a websocket
 * server, using the `connect` free function.
 *
 * @see WebsocketServer::Config for where to set the factory method on a websocket server.
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
   * @brief Close the endpoint.
   * @param close_code A type byte integer sent to the other endpoint.
   * @param reason An optional utf-8 encoded string.
   * @see https://datatracker.ietf.org/doc/html/rfc6455#section-7.1.2
   */
  void close(uint16_t close_code = 0, std::string_view reason = "");

  /**
   * @brief Send a message to the other end.
   * @todo We need a movable allocator-aware type to encapsulate a sequence of buffers
   */
  void send_message(BufferType&& buffer);

  /**
   * @brief A new connection has been made
   * @note must be threadsafe
   */
  virtual void on_connect() {}

  /**
   * @brief A callback that is called when a connection receives a new message.
   * @note Must be threadsafe
   *
   * Parameters
   * + `data` and `size` is the payload of the message. It must be decoded immediately,
   *   because the underlying buffer will be reused.
   */
  virtual void on_receive(std::span<const std::byte> payload) = 0;

  /**
   * @brief Should result in the resouce being freed
   * @note must be threadsafe
   */
  virtual void on_close(uint16_t code_code, std::string_view reason) {}

  /**
   * @brief Non-write errors result in the session being closed
   * @note must be threadsafe
   */
  virtual void on_error(WebsocketOperation operation, std::error_code ec) {}

  /**
   * @brief A `send_message` call is finished, and the buffer is being returned.
   * If not overriden, then the buffer will be deleted normally.
   */
  virtual void on_return_buffer(BufferType&& buffer) {}
};

/**
 * @brief Connect to an endpoint.
 *
 * @param session The instance that manages the client connections.
 * @param io_context An asio context for executing logic.
 * @param host Host to connect to.
 * @param port Port to connect to.
 *
 * A copy `session` is stored internally by the websocket driver, and the instance will
 * stay alive at least until the socket errors or is closed.
 */
void connect(std::shared_ptr<WebsocketSession> session, //
             boost::asio::io_context& io_context,       //
             std::string_view host,                     //
             uint16_t port);                            //

} // namespace niggly::net
