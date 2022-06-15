
// We know that `main.cpp` is going to be first in unity builds.
// Therefore, we include our precompiled header here, so that it
// is first in the unity (testcases) build.
#include "stdinc.hpp"

#include "niggly/async.hpp"
#include "niggly/net.hpp"
#include "niggly/utils/linear-allocator.hpp"

namespace niggly {

// class SessionImpl : public net::WebsocketSession {
//   void on_receive(const void* data, std::size_t size) override {
//     std::string_view s{static_cast<const char*>(data), size};
//     INFO("received: {}", s);
//     std::vector<char> buffer;
//     buffer.resize(size);
//     std::memcpy(buffer.data(), data, size);
//     send_message(std::move(buffer));
//   }

//   void on_close(uint16_t code, std::string_view reason) override {
//     INFO("closing session, code={}, reason='{}'", code, reason);
//   }

//   void on_error(net::WebsocketOperation operation, std::error_code ec) override {
//     LOG_ERR("error on op={}: {}", int(operation), ec.message());
//   }
// };

int main(int, char**) {
  // net::AsioExecutionContext pool;

  // net::WebsocketServer::Config config;
  // config.address = "0.0.0.0";
  // config.port = 8080;
  // config.dh_file = "assets/test-certificate/dh4096.pem";
  // config.certificate_chain_file = "assets/test-certificate/server.crt";
  // config.private_key_file = "assets/test-certificate/server.key";
  // config.session_factory = []() { return std::make_shared<SessionImpl>(); };

  // auto server = net::WebsocketServer{pool.io_context(), config};
  // auto ec = server.run();
  // if (ec) {
  //   LOG_ERR("starting rpc server: {}", ec.message());
  // }

  // pool.run();
  // pool.io_context().run(); // use this thread for processing requests as well

  return EXIT_SUCCESS;
}

} // namespace niggly

// Don't compile in main(...) if we're doing a testcase build
#ifndef CATCH_BUILD

#include <cstdio>

int main(int argc, char** argv) {
  printf("Hello world!\n");
  return niggly::main(argc, argv);
}

#endif
