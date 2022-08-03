

#include "stdinc.hpp"

#include "niggly/portable/asio/asio-execution-context.hpp"
#include "niggly/net/rpc/rpc-agent.hpp"

#include <catch2/catch.hpp>

namespace niggly::net::test {

/*
class RpcEchoServer : public net::RpcAgent<AsioExecutionContext::ExecutorType,
                                           AsioExecutionContext::SteadyTimerType> {
public:
  static std::function<void()>
  call_handler(std::shared_ptr<net::RpcAgent::CallContextType> context, // Rpc call context
               const void* data, // Rpc call payload, raw bytes
               std::size_t size) // Size of rpc call payload
  {
    CATCH_REQUIRE(context);
    INFO("Serving RPC request: {}", context->to_string());
    return []() {};
  }

  RpcEchoServer(net::AsioExecutionContext& exctx)
      : RpcAgent(exctx.get_executor(), // where echo requests are executed
                 call_handler,         // mapping of request-ids to function logic
                 [&exctx]() { return exctx.make_steady_timer(); }) // managing timeouts
  {}

  void on_error(net::WebsocketOperation operation, std::error_code ec) override {
    LOG_ERR("server error on op={}: {}", str(operation), ec.message());
  }
};

static void run_test_rpc_echo_server(uint16_t port) {
  boost::asio::io_context io_context;
  net::AsioExecutionContext pool{io_context, 2};

  net::WebsocketServer::Config config;
  config.address = "0.0.0.0";
  config.port = port;
  config.dh_file = "assets/test-certificate/dh4096.pem";
  config.certificate_chain_file = "assets/test-certificate/server.crt";
  config.private_key_file = "assets/test-certificate/server.key";
  config.session_factory = [&pool]() { return std::make_shared<RpcEchoServer>(pool); };

  auto server = net::WebsocketServer{io_context, config};
  auto ec = server.run();
  if (ec) {
    LOG_ERR("starting rpc server: {}", ec.message());
  }

  pool.run();

  auto client = std::make_shared<SessionClient>([&server]() { server.shutdown(); });
  connect(client, io_context, "localhost", port);

  io_context.run(); // use this thread for processing requests as well
}
*/

CATCH_TEST_CASE("rpc-echo-server", "[rpc-echo-server]") {

  CATCH_SECTION("rpc-echo-server") { TRACE("Hello rpc-echo-server"); }
}

// // The websocket server
// // RpcAgent instances
// // Handlers for all the functions
// class EchoServerSession : public RpcAgent<AsioExecutionContext::ExecutorType> {
// private:
// public:
//   // EchoServerSession(AsioExecutionContext::ExecutorType executor

//   // The echo function!
//   std::string echo(std::string_view);

//   // The square
//   int32_t square(int32_t value);
// };

// class EchoClient : private RpcAgent {
// private:
// public:
//   static std::shared_ptr<EchoClient> make(boost::asio::io_context& io_context,
//                                           std::string_view host, uint16_t post,
//                                           thunk_type completion = nullptr);
// };
} // namespace niggly::net::test
