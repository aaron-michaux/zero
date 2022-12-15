

#include "stdinc.hpp"

#include "niggly/portable/asio/asio-execution-context.hpp"
#include "niggly/net/websockets/websocket-server.hpp"
#include "niggly/net/rpc/rpc-agent.hpp"

#include <catch2/catch_all.hpp>

namespace niggly::net::test {

// An RPC method:
//  0. Call-id, unique for each call
//  1. Deserializer -> ParameterBlock
//  2. Async logic -- member function
//  3. Capture the result and reserialize -> BufferType (exceptions cannot be serialized)
//  4. Cancellation
//  5. Type erasure

class RpcHandler {
private:
  uint32_t call_id_{0};

public:
  auto call_id() const { return call_id_; }
};

// ----------------------------------------------------------------------------------- RpcEchoServer

class RpcEchoServer : public net::RpcAgent<AsioExecutionContext::ExecutorType,
                                           AsioExecutionContext::SteadyTimerType> {
public:
  static std::function<void()> call_handler(std::shared_ptr<CallContextType> context,
                                            std::span<const std::byte> payload) {
    CATCH_REQUIRE(context);
    const auto payload_str = niggly::str(payload);
    INFO("Serving RPC request: {}\n'''\n{}\n'''", context->to_string(), payload_str.data());
    return [context, data = make_send_buffer(payload)]() {
      context->finish_call({}, [data = std::move(data)](BufferType& buffer) {
        buffer.insert(end(buffer), cbegin(data), cend(data));
        return true;
      });
    };
  }

  RpcEchoServer(net::AsioExecutionContext& exctx)
      : RpcAgent{exctx.get_executor(), // where echo requests are executed
                 call_handler,         // mapping of request-ids to function logic
                 [&exctx]() { return exctx.make_steady_timer(); }} // managing timeouts
  {}

  void on_error(net::WebsocketOperation operation, std::error_code ec) override {
    LOG_ERR("server error on op={}: {}", str(operation), ec.message());
  }
};

// ----------------------------------------------------------------------------------- RpcEchoClient

class RpcEchoClient : public net::RpcAgent<AsioExecutionContext::ExecutorType,
                                           AsioExecutionContext::SteadyTimerType> {
private:
  std::function<void()> thunk_;

public:
  static std::function<void()> call_handler(std::shared_ptr<CallContextType> context,
                                            std::span<const std::byte> payload) {
    CATCH_REQUIRE(context);
    INFO("Serving RPC request: {}", context->to_string());
    CATCH_REQUIRE(false); // Should not be serving rpc requests from the server
    return []() {};
  }

  RpcEchoClient(net::AsioExecutionContext& exctx, std::function<void()> thunk)
      : RpcAgent{exctx.get_executor(), // where echo requests are executed
                 call_handler,         // mapping of request-ids to function logic
                 [&exctx]() { return exctx.make_steady_timer(); }}, // managing timeouts
        thunk_{std::move(thunk)} {}

  void on_connect() override {
    INFO("A new connection has been made");
    perform_rpc_call(
        0,    //
        1000, //
        [](net::BufferType& buffer) -> bool {
          buffer << "Hello World!";
          return true;
        }, //
        [this](net::Status status, std::span<const std::byte> payload) {
          INFO("received response({}):'''\n{}\n'''", status.error_code(),
               niggly::str(payload).data());
          thunk_();
        });
  }
};

// ------------------------------------------------------------------------ run_test_rpc_echo_server

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
  // config.session_factory = []() { return std::make_shared<zServerSession>(); };

  auto server = net::WebsocketServer{io_context, config};
  auto ec = server.run();
  if (ec) {
    LOG_ERR("starting rpc server: {}", ec.message());
  }

  pool.run();

  auto client = std::make_shared<RpcEchoClient>(pool, [&server]() { server.shutdown(); });
  //  auto client = std::make_shared<zSessionClient>([&server]() { server.shutdown(); });
  connect(client, io_context, "localhost", port); // returns a sender

  // auto snd = connect(client, io_context, "localhost", port)
  //          | client->echo("foo")
  //          | server->shutdown();

  io_context.run(); // use this thread for processing requests as well
}

CATCH_TEST_CASE("rpc-echo-server", "[rpc-echo-server]") {

  CATCH_SECTION("rpc-echo-server") {
    TRACE("Hello rpc-echo-server");
    run_test_rpc_echo_server(21492);
  }
}

} // namespace niggly::net::test
