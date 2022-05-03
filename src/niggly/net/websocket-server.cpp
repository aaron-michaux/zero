
#include "websocket-server.hpp"

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

namespace niggly::net
{
namespace asio  = boost::asio;
namespace beast = boost::beast;

namespace detail
{
   struct ServerCallbacks;
   class Session;
   class Listenter;
} // namespace detail

// This pimpl needs to come first
struct WebsocketSession::Pimpl
{
 public:
   observer_ptr<detail::Session> session = nullptr;
};

} // namespace niggly::net

namespace niggly::net::detail
{

// ------------------------------------------------------------------------------------- RpcResponse

class AsyncWriteState
{
 private:
   struct Pimpl
   {
      std::shared_ptr<Session> session;
      std::function<void(WebsocketBufferType&& buffer)> completion;
      WebsocketBufferType data;
      asio::const_buffer buffer;

      Pimpl(std::shared_ptr<Session> session_,
            std::function<void(WebsocketBufferType&& buffer)> completion_,
            WebsocketBufferType&& data_)
          : session{std::move(session_)}
          , completion{std::move(completion_)}
          , data{std::move(data_)}
          , buffer{asio::buffer(data.data(), data.size())}
      {}
   };
   std::shared_ptr<Pimpl> pimpl_;

 public:
   using value_type     = asio::const_buffer;
   using const_iterator = const asio::const_buffer*;

   AsyncWriteState(std::shared_ptr<Session> session,
                   std::function<void(WebsocketBufferType&& buffer)> completion,
                   WebsocketBufferType&& buffer)
       : pimpl_{
           std::make_shared<Pimpl>(std::move(session), std::move(completion), std::move(buffer))}
   {}

   const boost::asio::const_buffer* begin() const { return &pimpl_->buffer; }
   const boost::asio::const_buffer* end() const { return begin() + 1; }

   void on_complete(beast::error_code ec, std::size_t bytes_transferred)
   {
      if(ec) INFO("error on write: {}", ec.message());
      TRACE("write complete, {} transfered", bytes_transferred);
      // Recycle the buffer if anyone wants it
      if(pimpl_->completion) pimpl_->completion(std::move(pimpl_->data));
   }
};

// ------------------------------------------------------------------------------------
// ServerCallbacks

struct ServerCallbacks
{
   std::function<std::shared_ptr<WebsocketSession>()> session_factory;
   std::function<void(std::shared_ptr<WebsocketSession>)> on_new_session;
};

// ----------------------------------------------------------------------------------------- Session

// Echoes back all received WebSocket messages
class Session : public std::enable_shared_from_this<Session>
{
   const uint64_t id_;
   std::shared_ptr<ServerCallbacks> callbacks_;
   std::shared_ptr<WebsocketSession> external_session_ = nullptr; //! External facing session
   beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
   beast::flat_buffer buffer_;

 public:
   // Take ownership of the socket
   Session(asio::ip::tcp::socket&& socket,
           asio::ssl::context& ctx,
           std::shared_ptr<ServerCallbacks> callbacks,
           uint64_t id)
       : id_{id}
       , callbacks_{callbacks}
       , ws_{std::move(socket), ctx}
   {
      try {
         external_session_ = callbacks_->session_factory();
         if(external_session_ == nullptr)
            FATAL("callback `session_factory` returned an empty result");
         external_session_->pimpl_->session.reset(this);
         if(callbacks_->on_new_session) callbacks_->on_new_session(external_session_);
      } catch(std::exception& e) {
         FATAL("callback `session_factory` must not throw: {}", e.what());
      } catch(...) {
         FATAL("callback `session_factory` must not throw");
      }
      TRACE("session {} created", id_);
   }

   ~Session() { TRACE("session {} deleted", id_); }

   // Get on the correct executor
   void run()
   {
      // We need to be executing within a strand to perform async operations
      // on the I/O objects in this session. Although not strictly necessary
      // for single-threaded contexts, this example code is written to be
      // thread-safe by default.
      ::boost::asio::dispatch(ws_.get_executor(),
                              beast::bind_front_handler(&Session::on_run, shared_from_this()));
   }

   // Start the asynchronous operation
   void on_run()
   {
      // Set the timeout.
      beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

      // Perform the SSL handshake
      ws_.next_layer().async_handshake(
          asio::ssl::stream_base::server,
          beast::bind_front_handler(&Session::on_handshake, shared_from_this()));
   }

   void on_handshake(beast::error_code ec)
   {
      if(ec) {
         INFO("error on handshake: {}", ec.message());
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

   void on_accept(beast::error_code ec)
   {
      if(ec) {
         INFO("session {}, error on accept: {}", id_, ec.message());
         // TODO, on error
         external_session_->on_close(ec);
         return;
      }

      // Read a message
      do_read();
   }

   void do_read()
   {
      // Read a message into our buffer
      ws_.async_read(buffer_, beast::bind_front_handler(&Session::on_read, shared_from_this()));
   }

   void on_read(beast::error_code ec, std::size_t bytes_transferred)
   {
      boost::ignore_unused(bytes_transferred);

      // This indicates that the session was closed
      if(ec == beast::websocket::error::closed) {
         INFO("session {}, closed", id_);
         external_session_->on_close(std::error_code{});
         return;
      }

      if(ec) {
         INFO("session {}, error on read: {}", id_, ec.message());
         external_session_->on_close(ec);
         return;
      }

      const void* data = buffer_.data().data();
      std::size_t size = buffer_.data().size();
      try {
         external_session_->on_receive(data, size);
      } catch(std::exception& e) {
         FATAL("callback `on_receive` must not throw: {}", e.what());
      } catch(...) {
         FATAL("callback `on_receive` must not throw");
      }

      // Clear the buffer
      buffer_.consume(buffer_.size());
      do_read();
   }

   void async_write(WebsocketBufferType&& buffer,
                    std::function<void(WebsocketBufferType&& buffer)> completion)
   {
      auto state = AsyncWriteState{shared_from_this(), std::move(completion), std::move(buffer)};
      ws_.async_write(state, //
                      beast::bind_front_handler(&AsyncWriteState::on_complete, state));
   }
};

// ---------------------------------------------------------------------------------------- Listener

// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener>
{
 private:
   asio::io_context& ioc_;
   asio::ssl::context& ctx_;
   asio::ip::tcp::acceptor acceptor_;
   std::shared_ptr<ServerCallbacks> callbacks_;
   beast::error_code ec_;
   std::atomic<uint64_t> session_id_;

 public:
   Listener(asio::io_context& ioc,
            asio::ssl::context& ctx,
            asio::ip::tcp::endpoint endpoint,
            std::shared_ptr<ServerCallbacks> callbacks)
       : ioc_{ioc}
       , ctx_{ctx}
       , acceptor_{asio::make_strand(ioc)}
       , callbacks_{std::move(callbacks)}
       , ec_{}
       , session_id_{1}
   {
      // Open the acceptor
      acceptor_.open(endpoint.protocol(), ec_);
      if(ec_) return;

      // Allow address reuse
      acceptor_.set_option(asio::socket_base::reuse_address(true), ec_);
      if(ec_) return;

      // Bind to the server address
      acceptor_.bind(endpoint, ec_);
      if(ec_) return;

      // Start listening for connections
      acceptor_.listen(asio::socket_base::max_listen_connections, ec_);
      if(ec_) return;
   }

   // Start accepting incoming connections
   beast::error_code run()
   {
      if(!ec_) do_accept_();
      return ec_;
   }

 private:
   void do_accept_()
   {
      // The new connection gets its own strand
      acceptor_.async_accept(asio::make_strand(ioc_),
                             beast::bind_front_handler(&Listener::on_accept_, shared_from_this()));
   }

   void on_accept_(beast::error_code ec, asio::ip::tcp::socket socket)
   {
      if(ec) {
         INFO("rpc-server on-accept error: {}", ec.message());
      } else {
         // Create the session and run it
         std::make_shared<Session>(std::move(socket),
                                   ctx_,
                                   callbacks_,
                                   session_id_.fetch_add(1, std::memory_order_acq_rel))
             ->run();
      }

      // Accept another connection
      do_accept_();
   }
};

} // namespace niggly::net::detail

namespace niggly::net
{

// -------------------------------------------------------------------------------- WebsocketSession

WebsocketSession::WebsocketSession()
    : pimpl_{std::make_unique<Pimpl>()}
{
   //
}

WebsocketSession::~WebsocketSession() = default;

void WebsocketSession::send_message(WebsocketBufferType&& buffer)
{
   pimpl_->session->async_write(
       std::move(buffer),
       [this, session = pimpl_->session->shared_from_this()](WebsocketBufferType&& buffer) {
          on_return_buffer(std::move(buffer));
       });
}

// ------------------------------------------------------------------------------------------- Pimpl

struct WebsocketServer::Pimpl
{
   asio::io_context& io_context;
   asio::ssl::context context;
   string address                             = {};
   uint16_t port                              = 0;
   std::shared_ptr<detail::Listener> listener = nullptr;

   Pimpl(boost::asio::io_context& io_context_, const Config& config)
       : io_context{io_context_}
       , context{asio::ssl::context::tlsv12}
       , address{config.address}
       , port{config.port}
   {
      context.set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2
                          | asio::ssl::context::single_dh_use);
      context.use_certificate_chain_file(config.certificate_chain_file.data());
      context.use_private_key_file(config.private_key_file.data(), boost::asio::ssl::context::pem);
      context.use_tmp_dh_file(config.dh_file.data());
      context.set_password_callback(
          [](std::size_t, asio::ssl::context_base::password_purpose) { return "test"; });

      auto callbacks             = std::make_shared<detail::ServerCallbacks>();
      callbacks->session_factory = config.session_factory;
      callbacks->on_new_session  = config.on_new_session;

      listener = std::make_shared<detail::Listener>(
          io_context,
          context,
          asio::ip::tcp::endpoint{asio::ip::make_address(config.address.data()), config.port},
          std::move(callbacks));
   }
};

// ------------------------------------------------------------------------------------ Construction

WebsocketServer::WebsocketServer(boost::asio::io_context& io_context, const Config& config)
    : pimpl_{std::make_unique<Pimpl>(io_context, config)}
{}

WebsocketServer::~WebsocketServer() = default;

std::error_code WebsocketServer::run()
{
   // Create and launch a listening port
   if(pimpl_->listener == nullptr) return std::make_error_code(std::errc::not_connected);
   return pimpl_->listener->run();
}

} // namespace niggly::net
