
#include "websocket-server.hpp"
#include "websocket-session.hpp"

#include "niggly/utils.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace niggly::net {

namespace asio = boost::asio;
namespace beast = boost::beast;

namespace detail {
struct ServerCallbacks;
class Session;
class Listenter;
} // namespace detail

// This pimpl needs to come first
struct WebsocketSession::Pimpl {
private:
  std::mutex padlock_;
  std::weak_ptr<detail::Session> session_{};

public:
  void set_session(shared_ptr<detail::Session> session) {
    std::lock_guard lock{padlock_};
    session_ = session;
  }

  shared_ptr<detail::Session> session() {
    std::lock_guard lock{padlock_};
    return session_.lock();
  }
};

} // namespace niggly::net

namespace niggly::net::detail {

// ------------------------------------------------------------------------------------- RpcResponse

class AsyncWriteState {
private:
  struct Pimpl {
    std::shared_ptr<Session> session;
    std::function<void(BufferType&& buffer)> completion;
    BufferType data;
    asio::const_buffer buffer;

    Pimpl(std::shared_ptr<Session> session_, std::function<void(BufferType&& buffer)> completion_,
          BufferType&& data_)
        : session{std::move(session_)}, completion{std::move(completion_)}, data{std::move(data_)},
          buffer{asio::buffer(data.data(), data.size())} {}
  };
  std::shared_ptr<Pimpl> pimpl_;

public:
  using value_type = asio::const_buffer;
  using const_iterator = const asio::const_buffer*;

  AsyncWriteState(std::shared_ptr<Session> session,
                  std::function<void(BufferType&& buffer)> completion, BufferType&& buffer)
      : pimpl_{std::make_shared<Pimpl>(std::move(session), std::move(completion),
                                       std::move(buffer))} {}

  const boost::asio::const_buffer* begin() const { return &pimpl_->buffer; }
  const boost::asio::const_buffer* end() const { return begin() + 1; }

  void on_complete(beast::error_code ec, std::size_t bytes_transferred);
};

// --------------------------------------------------------------------------------- ServerCallbacks

struct ServerCallbacks {
  std::function<std::shared_ptr<WebsocketSession>()> session_factory;
};

// ----------------------------------------------------------------------------------------- Session

class Session : public std::enable_shared_from_this<Session> {
  const uint64_t id_{0};                                         //! server only
  std::shared_ptr<ServerCallbacks> callbacks_;                   //! server only
  std::shared_ptr<WebsocketSession> external_session_ = nullptr; //! External facing session
  beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
  beast::flat_buffer buffer_;
  std::chrono::milliseconds timeout_{30 * 1000};
  std::function<void(Session*)> on_close_thunk_;

  asio::ip::tcp::resolver resolver_; //! client only
  std::string host_;                 //! client only
  uint16_t port_{0};                 //! client only

public:
  // Takes ownership of the socket -- for building server-side sessions
  Session(uint64_t id, asio::ip::tcp::socket&& socket, asio::ssl::context& ctx)
      : id_{id}, ws_{std::move(socket), ctx}, resolver_{ws_.get_executor()} {}

  Session(asio::io_context& ioc, asio::ssl::context& ctx)
      : ws_{asio::make_strand(ioc), ctx}, resolver_{asio::make_strand(ioc)} {}

  ~Session() {
    if (on_close_thunk_)
      on_close_thunk_(this);
  }

  static std::shared_ptr<Session>
  make_server_side_session(uint64_t id, asio::ip::tcp::socket&& socket, asio::ssl::context& ctx,
                           std::shared_ptr<ServerCallbacks> callbacks) {
    auto session = std::make_shared<Session>(id, std::move(socket), ctx);
    session->callbacks_ = std::move(callbacks);
    try {
      session->external_session_ = session->callbacks_->session_factory();
      if (session->external_session_ == nullptr)
        FATAL("callback `session_factory` returned an empty result");
      session->external_session_->pimpl_->set_session(session);
    } catch (std::exception& e) {
      FATAL("callback `session_factory` must not throw: {}", e.what());
    } catch (...) {
      FATAL("callback `session_factory` must not throw");
    }
    TRACE("server side session created, id={}", id);
    return session;
  }

  // ws_.next_layer().set_verify_mode(asio::ssl::verify_none);
  static void client_connect(std::shared_ptr<WebsocketSession> ws_session,
                             asio::io_context& io_context, std::string_view host, uint16_t port) {
    static asio::ssl::context ctx{asio::ssl::context::tlsv12_client};

    assert(ws_session != nullptr);

    auto internal_session = std::make_shared<Session>(io_context, ctx);
    internal_session->external_session_ = ws_session;
    ws_session->pimpl_->set_session(internal_session);

    internal_session->connect(host, port);
  }

  void close(uint16_t close_code, std::string_view reason) {
    auto ec = beast::error_code{};
    auto close_reason = beast::websocket::close_reason{
        beast::websocket::close_code{close_code}, beast::string_view{reason.data(), reason.size()}};

    ws_.async_close(close_reason, [ptr = shared_from_this()](beast::error_code ec) {
      if (ec == beast::websocket::error::closed)
        ptr->on_close();
      else
        ptr->external_session_->on_error(WebsocketOperation::CLOSE, ec);
    });
  }

  void cancel_socket() {
    asio::post(ws_.get_executor(),
               [ptr = shared_from_this()]() { beast::get_lowest_layer(ptr->ws_).cancel(); });
  }

  void on_close() {
    uint16_t code = ws_.reason().code;
    auto reason = ws_.reason().reason;
    external_session_->on_close(code, std::string_view{reason.data(), reason.size()});
  }

  // @{ Client side functions
  void connect(std::string_view host, uint16_t port) {
    // Save these for later
    host_ = std::string{std::cbegin(host), std::cend(host)};
    port_ = port;

    // Look up the domain name
    resolver_.async_resolve(host.data(), std::to_string(port_).data(),
                            beast::bind_front_handler(&Session::on_resolve, shared_from_this()));
  }

  void on_resolve(beast::error_code ec, asio::ip::tcp::resolver::results_type results) {
    if (ec) {
      on_error(WebsocketOperation::CONNECT, ec);
      return;
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(timeout_); // TODO, config

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(ws_).async_connect(
        results, beast::bind_front_handler(&Session::on_client_connect, shared_from_this()));
  }

  void on_client_connect(beast::error_code ec,
                         asio::ip::tcp::resolver::results_type::endpoint_type ep) {
    if (ec) {
      on_error(WebsocketOperation::CONNECT, ec);
      return;
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(timeout_);

// Set SNI Hostname (many hosts need this to handshake successfully)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    const bool set_tls_successful =
        SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str());
#pragma GCC diagnostic pop
    if (!set_tls_successful) {
      ec = beast::error_code{static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
      on_error(WebsocketOperation::CONNECT, ec);
      return;
    }

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    host_ += ':' + std::to_string(ep.port());

    // Perform the SSL handshake
    ws_.next_layer().async_handshake(
        asio::ssl::stream_base::client,
        beast::bind_front_handler(&Session::on_client_handshake, shared_from_this()));
  }

  void on_client_handshake(beast::error_code ec) {
    if (ec) {
      on_error(WebsocketOperation::HANDSHAKE, ec);
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
    ws_.async_handshake(host_, "/",
                        beast::bind_front_handler(&Session::on_accept, shared_from_this()));
  }
  // @}

  // @{ Server side functions
  // Get on the correct executor
  void run_server_session(std::function<void(Session*)> on_close_thunk) {
    on_close_thunk_ = std::move(on_close_thunk); // deletes server-side resources

    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.
    boost::asio::dispatch(
        ws_.get_executor(),
        beast::bind_front_handler(&Session::on_run_server_session, shared_from_this()));
  }

  // Start the asynchronous operation
  void on_run_server_session() {
    // Set the timeout.
    beast::get_lowest_layer(ws_).expires_after(timeout_);

    // Perform the SSL handshake
    ws_.next_layer().async_handshake(
        asio::ssl::stream_base::server,
        beast::bind_front_handler(&Session::on_server_handshake, shared_from_this()));
  }

  void on_server_handshake(beast::error_code ec) {
    if (ec) {
      on_error(WebsocketOperation::HANDSHAKE, ec);
      return;
    }

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(ws_).expires_never();

    // Set suggested timeout settings for the websocket
    ws_.set_option(beast::websocket::stream_base::timeout::suggested(beast::role_type::server));

    // Set a decorator to change the Server of the handshake
    ws_.set_option(
        beast::websocket::stream_base::decorator([](beast::websocket::response_type& res) {
          res.set(beast::http::field::server,
                  std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async-ssl");
        }));

    // Accept the websocket handshake
    ws_.async_accept(beast::bind_front_handler(&Session::on_accept, shared_from_this()));
  }
  // @}

  void on_accept(beast::error_code ec) {
    if (ec) {
      on_error(WebsocketOperation::ACCEPT, ec);
      return;
    }

    external_session_->on_connect();

    // Read a message
    do_read();
  }

  void do_read() {
    // Read a message into our buffer
    ws_.async_read(buffer_, beast::bind_front_handler(&Session::on_read, shared_from_this()));
  }

  void on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This indicates that the session was closed
    if (ec == beast::websocket::error::closed) {
      on_close();
      return;
    }

    if (ec) {
      on_error(WebsocketOperation::READ, ec);
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

    // Clear the buffer
    buffer_.consume(buffer_.size());
    do_read();
  }

  void async_write(BufferType&& buffer, std::function<void(BufferType&& buffer)> completion) {
    auto state = AsyncWriteState{shared_from_this(), std::move(completion), std::move(buffer)};
    ws_.async_write(state, //
                    beast::bind_front_handler(&AsyncWriteState::on_complete, state));
  }

  void on_error(WebsocketOperation operation, std::error_code ec) {
    external_session_->on_error(operation, ec);
  }
};

// --------------------------------------------------------------------------------- AsyncWriteState

void AsyncWriteState::on_complete(beast::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    pimpl_->session->on_error(WebsocketOperation::WRITE, ec);
  }

  // Recycle the buffer if anyone wants it
  if (pimpl_->completion)
    pimpl_->completion(std::move(pimpl_->data));
}

// ---------------------------------------------------------------------------------------- Listener

// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener> {
private:
  asio::io_context& ioc_;
  asio::ssl::context& ctx_;
  asio::ip::tcp::acceptor acceptor_;
  std::shared_ptr<ServerCallbacks> callbacks_;
  beast::error_code ec_;
  std::atomic<uint64_t> session_id_;

  /**
   * When a session destructs, a callback should delete from this session
   */
  std::mutex padlock_;
  std::unordered_map<Session*, std::weak_ptr<Session>> sessions_;
  bool is_shutdown_ = false;

public:
  Listener(asio::io_context& ioc, asio::ssl::context& ctx, asio::ip::tcp::endpoint endpoint,
           std::shared_ptr<ServerCallbacks> callbacks)
      : ioc_{ioc}, ctx_{ctx}, acceptor_{asio::make_strand(ioc)},
        callbacks_{std::move(callbacks)}, ec_{}, session_id_{1} {
    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec_);
    if (ec_)
      return;

    // Allow address reuse
    acceptor_.set_option(asio::socket_base::reuse_address(true), ec_);
    if (ec_)
      return;

    // Bind to the server address
    acceptor_.bind(endpoint, ec_);
    if (ec_)
      return;

    // Start listening for connections
    acceptor_.listen(asio::socket_base::max_listen_connections, ec_);
    if (ec_)
      return;
  }

  // Start accepting incoming connections
  beast::error_code run() {
    if (!ec_)
      do_accept_();
    return ec_;
  }

  void shutdown() {
    {
      std::lock_guard lock{padlock_};
      is_shutdown_ = true;
    }
    asio::post(acceptor_.get_executor(), [this]() { finish_shutdown_(); });
  }

private:
  void do_accept_() {
    // The new connection gets its own strand
    acceptor_.async_accept(asio::make_strand(ioc_),
                           beast::bind_front_handler(&Listener::on_accept_, shared_from_this()));
  }

  void on_accept_(beast::error_code ec, asio::ip::tcp::socket socket) {
    if (ec) {
      INFO("rpc-server on-accept error: {}", ec.message());
    } else {
      // Create the session and run it
      auto session = Session::make_server_side_session(
          session_id_.fetch_add(1, std::memory_order_acq_rel), std::move(socket), ctx_, callbacks_);

      bool is_shutdown = false;

      {
        std::lock_guard lock{padlock_};
        is_shutdown = is_shutdown_;
        if (!is_shutdown)
          sessions_.insert({session.get(), session});
      }

      if (is_shutdown) {
        session->cancel_socket();
      } else {
        session->run_server_session([this](Session* session) mutable { remove_session_(session); });
      }
    }

    // Accept another connection
    do_accept_();
  }

  void remove_session_(Session* session) {
    std::lock_guard lock{padlock_};
    sessions_.erase(session);
  }

  void finish_shutdown_() {
    acceptor_.cancel(); // stop listening

    decltype(sessions_) sessions;
    { // clean out the current sessions
      std::lock_guard lock{padlock_};
      using std::swap;
      swap(sessions, sessions_);
    }

    for (auto& session : sessions) {
      auto ptr = session.second.lock();
      if (ptr)
        ptr->cancel_socket();
    }
  }
};

} // namespace niggly::net::detail

namespace niggly::net {

// -------------------------------------------------------------------------------- WebsocketSession

WebsocketSession::WebsocketSession() : pimpl_{std::make_unique<Pimpl>()} {}

WebsocketSession::~WebsocketSession() = default;

void WebsocketSession::close(uint16_t close_code, std::string_view reason) {
  auto ptr = pimpl_->session();
  if (ptr) {
    ptr->close(close_code, reason);
  } else {
    // TODO, on_error
  }
}

void WebsocketSession::send_message(BufferType&& buffer) {
  auto ptr = pimpl_->session();
  if (ptr) {
    ptr->async_write(std::move(buffer), [this, session = ptr](BufferType&& buffer) {
      on_return_buffer(std::move(buffer));
    });
  } else {
    // TODO, on error
  }
}

void connect(std::shared_ptr<WebsocketSession> session, asio::io_context& io_context,
             std::string_view host, uint16_t port) {
  detail::Session::client_connect(std::move(session), io_context, host, port);
}

// ------------------------------------------------------------------------------------------- Pimpl

struct WebsocketServer::Pimpl {
  asio::io_context& io_context;
  asio::ssl::context context;
  string address = {};
  uint16_t port = 0;
  std::shared_ptr<detail::Listener> listener = nullptr;

  Pimpl(boost::asio::io_context& io_context_, const Config& config)
      : io_context{io_context_}, context{asio::ssl::context::tlsv12}, address{config.address},
        port{config.port} {
    context.set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
                        asio::ssl::context::single_dh_use);
    context.use_certificate_chain_file(config.certificate_chain_file.data());
    context.use_private_key_file(config.private_key_file.data(), boost::asio::ssl::context::pem);
    context.use_tmp_dh_file(config.dh_file.data());
    context.set_password_callback(
        [](std::size_t, asio::ssl::context_base::password_purpose) { return "test"; });

    auto callbacks = std::make_shared<detail::ServerCallbacks>();
    callbacks->session_factory = config.session_factory;

    listener = std::make_shared<detail::Listener>(
        io_context, context,
        asio::ip::tcp::endpoint{asio::ip::make_address(config.address.data()), config.port},
        std::move(callbacks));
  }
};

// ------------------------------------------------------------------------------------ Construction

WebsocketServer::WebsocketServer(boost::asio::io_context& io_context, const Config& config)
    : pimpl_{std::make_unique<Pimpl>(io_context, config)} {}

WebsocketServer::~WebsocketServer() = default;

std::error_code WebsocketServer::run() {
  // Create and launch a listening port
  if (pimpl_->listener == nullptr)
    return std::make_error_code(std::errc::not_connected);
  return pimpl_->listener->run();
}

void WebsocketServer::shutdown() {
  TRACE("post shutdown");
  pimpl_->listener->shutdown();
}

} // namespace niggly::net
