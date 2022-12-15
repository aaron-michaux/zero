
#include "niggly/async.hpp"
#include "niggly/utils/linear-allocator.hpp"

#include "stdinc.hpp"

#include <catch2/catch_all.hpp>

#include <boost/asio.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/execution.hpp>
#include <boost/asio/static_thread_pool.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <numeric>
#include <vector>

namespace niggly::coroutines {
/*
struct suspend_always
{
   bool await_ready() noexcept { return false; }
   void await_suspend(coroutine_handle<>) noexcept {}
   void await_resume() noexcept {}
};

struct suspend_never
{
   bool await_ready() noexcept { return true; }
   void await_suspend(coroutine_handle<>) noexcept {}
   void await_resume() noexcept {}
};

*/

// template<typename CompletionHandler> struct Async
// {
//    // Action
//    CompletionHandler handler_;
//    void completed(CompletionHandler handler) { handler_ = std::move(handler); }
//    CompletionHandler completed() { return handler_; }
//    void get_results() {}

//    // Info
//    uint32_t id() { return 1; }
//    AsyncStatus status() { return AsyncStatus::Started; }
//    std::error_code error_code();
//    void cancel();
//    void close();
// }

} // namespace niggly::coroutines

namespace niggly::async {

// ------------------------------------------------------------------- TEST_CASE
//
CATCH_TEST_CASE("Coroutine1", "[coroutine-1]") {}

} // namespace niggly::async
