
#include "stdinc.hpp"

// #include "niggly/net/execution-context.hpp"
// #include "niggly/net/websocket-server.hpp"

// namespace niggly::example
// {
// class SessionImpl : public net::WebsocketSession
// {
//    void on_receive(const void* data, std::size_t size) override
//    {
//       std::string_view s{static_cast<const char*>(data), size};
//       INFO("received: {}", s);
//       std::vector<char> buffer;
//       buffer.resize(size);
//       std::memcpy(buffer.data(), data, size);
//       send_message(std::move(buffer));
//    }

//    void on_close(std::error_code ec) override { INFO("closing session: {}", ec.message()); }
// };

// void test_async_echo_server()
// {
//    net::ExecutionContext pool;

//    net::WebsocketServer::Config config;
//    config.address                = "0.0.0.0";
//    config.port                   = 8080;
//    config.dh_file                = "assets/test-certificate/dh4096.pem";
//    config.certificate_chain_file = "assets/test-certificate/server.crt";
//    config.private_key_file       = "assets/test-certificate/server.key";
//    config.session_factory        = []() { return std::make_shared<SessionImpl>(); };

//    auto server = net::WebsocketServer{pool.io_context(), config};
//    auto ec     = server.run();
//    if(ec) { LOG_ERR("starting rpc server: {}", ec.message()); }

//    pool.run();
//    pool.io_context().run(); // use this thread for processing requests as well
// }

// } // namespace niggly::example
