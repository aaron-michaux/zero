
#include "niggly/async.hpp"
#include "niggly/utils/linear-allocator.hpp"

#include <boost/asio.hpp>

namespace niggly::example
{

using namespace niggly::async;

static void futures_example()
{
   LinearAllocator<void> allocator; // Allocator optional

   boost::asio::io_context io_context;

   // Threadpool with 4 threads
   constexpr std::size_t num_threads = 4;
   auto wg                           = boost::asio::make_work_guard(io_context);
   std::vector<std::thread> pool;
   pool.reserve(num_threads);
   for(std::size_t i = 0; i < num_threads; ++i)
      pool.emplace_back([&io_context]() { io_context.run(); });

   asio::ExecutionBroker executor{io_context};

   auto f1 = async_later(executor, allocator, std::chrono::milliseconds{444}, []() {
      return std::make_unique<int>(42);
   });

   auto f2
       = async_later(executor, std::chrono::seconds{1}, []() { return std::make_unique<int>(43); });

   auto f3
       = async_later(executor, std::chrono::seconds{1}, []() { return std::make_unique<int>(44); });

   auto f4
       = f3.then(executor, allocator, [](std::unique_ptr<int> value) -> int { return *value + 1; });

   f1.cancel();
   f1.wait();
   f2.wait();

   try {
      TRACE("the results was {}", *f1.get());
      assert(false); // should have thrown
   } catch(std::future_error& e) {
      assert(e.code() == std::future_errc::broken_promise);
      TRACE("f1 was cancelled, as expected");
   }

   f4.wait();
   TRACE("f4 results was {}", f4.get());

   // Finish work... and cleanup threadpool
   wg.reset();
   for(auto& thread : pool) thread.join();
   TRACE("Done");
}

} // namespace niggly::example
