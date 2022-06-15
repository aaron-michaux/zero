

#include "stdinc.hpp"

#include "niggly/portable/asio/asio-execution-context.hpp"
#include "niggly/net/rpc/rpc-agent.hpp"

#include <catch2/catch.hpp>

namespace niggly::test {

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
} // namespace niggly::test
