
#include "stdinc.hpp"

#include "niggly/net/asio-execution-context.hpp"
#include "niggly/net/websockets/websocket-server.hpp"

namespace niggly::example {

// ----------------------------------------------------------------------------------- ServerSession

class ServerSession : public net::WebsocketSession {
public:
  void on_connect() override { INFO("server created a new connection"); }

  void on_receive(const void* data, std::size_t size) override {
    std::string_view s{static_cast<const char*>(data), size};
    INFO("server received: {}", s);
    std::vector<char> buffer;
    buffer.resize(size);
    std::memcpy(buffer.data(), data, size);
    send_message(std::move(buffer));
  }

  void on_close(uint16_t code, std::string_view reason) override {
    INFO("server closing session, code={}, reason='{}'", code, reason);
  }

  void on_error(net::WebsocketOperation operation, std::error_code ec) override {
    LOG_ERR("server error on op={}: {}", int(operation), ec.message());
  }
};

// ---------------------------------------------------------------------------------- Session Client

class SessionClient : public net::WebsocketSession {
private:
  thunk_type on_close_thunk_;

public:
  SessionClient(thunk_type on_close_thunk) : on_close_thunk_{on_close_thunk} {}

  void on_connect() override {
    INFO("client connected");
    send_message(net::make_send_buffer("Hello World!"));
  }

  void on_receive(const void* data, std::size_t size) override {
    std::string_view s{static_cast<const char*>(data), size};
    INFO("client received: {}", s);
    close(0, "orderly shutdown");
  }

  void on_close(uint16_t code, std::string_view reason) override {
    INFO("client closing session, code={}, reason='{}'", code, reason);
    on_close_thunk_();
  }

  void on_error(net::WebsocketOperation operation, std::error_code ec) override {
    LOG_ERR("error on op={}: {}", str(operation), ec.message());
  }
};

// --------------------------------------------------------------------------------- run-test-server

static void run_test_server() {
  const uint16_t port = 17002;

  net::WebsocketServer::Config config;
  config.address = "0.0.0.0";
  config.port = port;
  config.dh_file = "assets/test-certificate/dh4096.pem";
  config.certificate_chain_file = "assets/test-certificate/server.crt";
  config.private_key_file = "assets/test-certificate/server.key";
  config.session_factory = []() {
    // Create `ServerSession` instances to manage connections when a client creates a new connection
    return std::make_shared<ServerSession>();
  };

  // An execution context for running server/client functions
  net::AsioExecutionContext pool{2};

  auto server = net::WebsocketServer{pool.io_context(), config};
  auto ec = server.run();
  if (ec) {
    LOG_ERR("starting rpc server: {}", ec.message());
    return;
  }

  pool.run();

  auto client = std::make_shared<SessionClient>([&server]() {
    // Tell `server` to shutdown when the client connection closes
    server.shutdown();
  });
  connect(client, pool.io_context(), "localhost", port); // Connect to server

  pool.io_context().run(); // use this thread for processing requests as well
}

} // namespace niggly::example
