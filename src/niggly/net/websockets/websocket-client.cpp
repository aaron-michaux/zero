
#ifdef ZAPPYZAPPYZAPPY

#include "websocket-client.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

// #include <boost/asio/buffer.hpp>
// #include <boost/asio/dispatch.hpp>
// #include <boost/asio/io_context.hpp>
// #include <boost/asio/ssl/context.hpp>
// #include <boost/asio/strand.hpp>

namespace niggly::net {

namespace asio = boost::asio;
namespace beast = boost::beast;

// Report a failure
static void fail(beast::error_code ec, char const* what) {
  std::cerr << what << ": " << ec.message() << "\n";
}

// Sends a WebSocket message and prints the response
class ClientSession : public std::enable_shared_from_this<ClientSession> {
private:
  asio::ip::tcp::resolver resolver_{};
  beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_{};
  beast::flat_buffer buffer_{};
  std::string host_{};
  uint16_t port_{0};
  std::string text_{};
  std::chrono::milliseconeds timeout_{30 * 1000};

public:
  // Resolver and socket require an io_context
  explicit ClientSession(boost::asio::io_context& ioc, ssl::context& ctx)
      : resolver_{asio::make_strand(ioc)}, ws_{asio::make_strand(ioc), ctx} {}

  // Start the asynchronous operation
  void run(std::string_view host, uint16_t port, std::string_view text) {
    // Save these for later
    host_ = std::string{std::cbegin(host), std::cend(host)};
    port_ = port;
    text_ = std::string{std::cbegin(text), std::cend(text)};

    // Look up the domain name
    resolver_.async_resolve(
        host.data(), std::to_string(port_).data(),
        beast::bind_front_handler(&ClientSession::on_resolve, shared_from_this()));
  }

  void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec) {
      return fail(ec, "resolve");
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(timeout_); // TODO, config

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(ws_).async_connect(
        results, beast::bind_front_handler(&ClientSession::on_connect, shared_from_this()));
  }

  void on_connect(beast::error_code ec, asio::ip::tcp::resolver::results_type::endpoint_type ep) {
    if (ec) {
      return fail(ec, "connect");
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str())) {
      ec = beast::error_code{static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
      return fail(ec, "connect");
    }

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    host_ += ':' + std::to_string(ep.port());

    // Perform the SSL handshake
    ws_.next_layer().async_handshake(
        asio::ssl::stream_base::client,
        beast::bind_front_handler(&ClientSession::on_ssl_handshake, shared_from_this()));
  }

  void on_ssl_handshake(beast::error_code ec) {
    if (ec) {
      INFO("error on ssl-handshake: {}", ec.message());
      return;
    }

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(ws_).expires_never();

    // Set suggested timeout settings for the websocket
    ws_.set_option(beast::websocket::stream_base::timeout::suggested(beast::role_type::client));

    // Set a decorator to change the User-Agent of the handshake
    ws_.set_option(
        beast::websocket::stream_base::decorator([](beast::websocket::request_type& req) {
          req.set(beast::http::field::user_agent,
                  std::string{BOOST_BEAST_VERSION_STRING} + " websocket-client-async-ssl");
        }));

    // Perform the websocket handshake
    ws_.async_handshake(
        host_, "/", beast::bind_front_handler(&ClientSession::on_handshake, shared_from_this()));
  }

  void on_handshake(beast::error_code ec) {
    if (ec) {
      INFO("error on handshake: {}", ec.message());
      return;
    }

    // Read a message
    do_read();
  }

  void do_read() {
    // Read a message into our buffer
    ws_.async_read(buffer_, beast::bind_front_handler(&ClientSession::on_read, shared_from_this()));
  }

  void on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This indicates that the session was closed
    if (ec == beast::websocket::error::closed) {
      TRACE("session {}, closed", id_);
      external_session_->on_close(std::error_code{});
      return;
    }

    if (ec) {
      TRACE("session {}, error on read: {}", id_, ec.message());
      external_session_->on_close(ec);
      return;
    }

    const void* data = buffer_.data().data();
    std::size_t size = buffer_.data().size();
    try {
      external_session_->on_receive(data, size);
    } catch (std::exception& e) {
      FATAL("callback `on_receive` must not throw: {}", e.what());
    } catch (...) {
      FATAL("callback `on_receive` must not throw");
    }

    buffer_.consume(buffer_.size()); // Clear the buffer
    do_read();                       // Read another message
  }

  void async_write(BufferType&& buffer, std::function<void(BufferType&& buffer)> completion) {
    auto state = AsyncWriteState{shared_from_this(), std::move(completion), std::move(buffer)};
    ws_.async_write(state, //
                    beast::bind_front_handler(&AsyncWriteState::on_complete, state));
  }
};

struct WebsocketClient::Pimpl : public std::enable_shared_from_this<WebsocketClient::Pimpl> {
  asio::io_context& io_context;
  asio::ssl::context context;

  Pimpl(boost::asio::io_context& io_context_) : io_context{io_context_} {}

  void async_write(BufferType&& buffer, std::function<void(BufferType&& buffer)> completion) {
    auto state = AsyncWriteState{shared_from_this(), std::move(completion), std::move(buffer)};
    ws_.async_write(state, //
                    beast::bind_front_handler(&AsyncWriteState::on_complete, state));
  }
};

// --------------------------------------------------------------------------------- WebsocketClient

WebsocketClient::WebsocketClient(boost::asio::io_context& io_context)
    : pimpl_{std::make_shared<Pimpl>(io_context)} {}

WebsocketClient::~WebsocketClient() = delete;

void WebsocketClient::connect(std::string_view url) {}

void WebsocketClient::send_message(BufferType&& buffer) {
  pimpl_->async_write(std::move(buffer),
                      [this, session = pimpl_->shared_from_this()](BufferType&& buffer) {
                        on_return_buffer(std::move(buffer));
                      });
}

} // namespace niggly::net
#endif
