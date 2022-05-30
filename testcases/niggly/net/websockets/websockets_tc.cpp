
#include "stdinc.hpp"

#include "niggly/net/asio-execution-context.hpp"
#include "niggly/net/websockets/websocket-server.hpp"

#include <catch2/catch.hpp>

namespace niggly::test {

class SessionImpl : public net::WebsocketSession {
  void on_connect() override { INFO("created a new connection"); }

  void on_receive(const void* data, std::size_t size) override {
    std::string_view s{static_cast<const char*>(data), size};
    INFO("received: {}", s);
    std::vector<char> buffer;
    buffer.resize(size);
    std::memcpy(buffer.data(), data, size);
    send_message(std::move(buffer));
  }

  void on_close(uint16_t code, std::string_view reason) override {
    INFO("closing session, code={}, reason='{}'", code, reason);
  }

  void on_error(net::WebsocketOperation operation, std::error_code ec) override {
    LOG_ERR("error on op={}: {}", int(operation), ec.message());
  }
};

static void run_test_server(uint16_t port) {
  net::AsioExecutionContext pool;

  net::WebsocketServer::Config config;
  config.address = "0.0.0.0";
  config.port = port;
  config.dh_file = "assets/test-certificate/dh4096.pem";
  config.certificate_chain_file = "assets/test-certificate/server.crt";
  config.private_key_file = "assets/test-certificate/server.key";
  config.session_factory = []() { return std::make_shared<SessionImpl>(); };

  auto server = net::WebsocketServer{pool.io_context(), config};
  auto ec = server.run();
  if (ec) {
    LOG_ERR("starting rpc server: {}", ec.message());
  }

  pool.run();
  server.shutdown();
  pool.io_context().run(); // use this thread for processing requests as well
}

CATCH_TEST_CASE("websockets-server", "[websockets-server]") {

  constexpr uint16_t k_port = 28081;

  run_test_server(k_port);

  CATCH_SECTION("websockets-server") {}
}

} // namespace niggly::test
