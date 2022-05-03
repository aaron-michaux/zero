
#include "niggly/async.hpp"
#include "niggly/utils/linear-allocator.hpp"

#include "stdinc.hpp"

#include <catch2/catch.hpp>

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

namespace niggly::async
{

template<typename Allocator, typename ExecutionBroker>
void run_test_async_with_allocator_(Allocator allocator, ExecutionBroker executor)
{
   auto f1 = async(executor, allocator, []() { return std::make_unique<int>(42); });
   auto f2
       = f1.then(executor, allocator, [](std::unique_ptr<int> value) -> int { return *value + 1; });
   f2.wait();
   CATCH_REQUIRE(f2.is_ready());
   CATCH_REQUIRE(f2.get() == 43);
}

template<typename Allocator, typename ExecutionBroker>
void run_test_async_later_with_allocator_(Allocator allocator, ExecutionBroker executor)
{
   // -- -- //

   auto f1 = async_later(executor, allocator, std::chrono::milliseconds{444}, []() {
      return std::make_unique<int>(42);
   });

   auto f2
       = async_later(executor, std::chrono::seconds{1}, []() { return std::make_unique<int>(43); });

   auto f3
       = async_later(executor, std::chrono::seconds{1}, []() { return std::make_unique<int>(44); });

   // Note, the executor can be set upt to run  the `then` clause
   // in whatever execution context you want. If you want it to run
   // immediately (if the results is already set) or in the thread
   // that sets the promise -- the executor can be written to do that.

   auto f4
       = f3.then(executor, allocator, [](std::unique_ptr<int> value) -> int { return *value + 1; });

   f1.cancel();
   f1.wait();
   f2.wait();

   try {
      f1.get();             // should throw
      CATCH_REQUIRE(false); // should have thrown
   } catch(std::future_error& e) {
      CATCH_REQUIRE(e.code() == std::future_errc::broken_promise);
   }

   f4.wait();
}

template<typename ExecutionBroker> void run_test_async_(ExecutionBroker executor)
{
   run_test_async_with_allocator_(LinearAllocator<void>{}, executor);
   run_test_async_with_allocator_(SingleThreadLinearAllocator<void>{}, executor);
   // run_test_async_later_with_allocator_(LinearAllocator<void>{}, executor);
   // run_test_async_later_with_allocator_(SingleThreadLinearAllocator<void>{}, executor);
}

// ------------------------------------------------------------------- TEST_CASE
//
CATCH_TEST_CASE("FutureVoid", "[future-void]")
{
   CATCH_SECTION("future-void-ex")
   {
      Promise<void> promise;
      auto future = promise.get_future();
      CATCH_REQUIRE(future.valid());
      CATCH_REQUIRE(future.is_ready() == false);
      try {
         throw std::runtime_error{"foo"};
      } catch(...) {
         promise.set_exception(std::current_exception());
      }
      CATCH_REQUIRE(future.is_ready());
      CATCH_REQUIRE(future.has_exception());
      CATCH_REQUIRE(future.valid());
      try {
         future.get();
         CATCH_REQUIRE(false); // we should throw
      } catch(std::runtime_error& e) {
         CATCH_REQUIRE(e.what() == std::string_view{"foo"});
      }
      CATCH_REQUIRE(!future.valid());
      CATCH_REQUIRE(!future.is_ready());
   }

   CATCH_SECTION("future-void-set")
   {
      Promise<void> promise;
      auto future = promise.get_future();
      CATCH_REQUIRE(future.valid());
      CATCH_REQUIRE(future.is_ready() == false);
      promise.set_value();
      CATCH_REQUIRE(future.is_ready());
      CATCH_REQUIRE(!future.has_exception());
      CATCH_REQUIRE(future.valid());
      future.get();
      CATCH_REQUIRE(!future.valid());
      CATCH_REQUIRE(!future.is_ready());
   }

   CATCH_SECTION("future-int-ex")
   {
      Promise<int> promise;
      auto future = promise.get_future();
      CATCH_REQUIRE(future.valid());
      CATCH_REQUIRE(future.is_ready() == false);
      try {
         throw std::runtime_error{"foo"};
      } catch(...) {
         promise.set_exception(std::current_exception());
      }
      CATCH_REQUIRE(future.is_ready());
      CATCH_REQUIRE(future.has_exception());
      CATCH_REQUIRE(future.valid());
      try {
         promise.set_value(42);
         CATCH_REQUIRE(false);
      } catch(std::future_error& e) {
         CATCH_REQUIRE(e.code() == std::future_errc::promise_already_satisfied);
      } catch(...) {
         CATCH_REQUIRE(false);
      }
      try {
         future.get();
         CATCH_REQUIRE(false); // we should throw
      } catch(std::runtime_error& e) {
         CATCH_REQUIRE(e.what() == std::string_view{"foo"});
      }
      CATCH_REQUIRE(!future.valid());
      CATCH_REQUIRE(!future.is_ready());
   }

   CATCH_SECTION("future-int-set")
   {
      Promise<int> promise;
      auto future = promise.get_future();
      CATCH_REQUIRE(future.valid());
      CATCH_REQUIRE(future.is_ready() == false);
      promise.set_value(42);
      CATCH_REQUIRE(future.is_ready());
      CATCH_REQUIRE(!future.has_exception());
      CATCH_REQUIRE(future.valid());
      try {
         promise.set_value(43);
         CATCH_REQUIRE(false);
      } catch(std::future_error& e) {
         CATCH_REQUIRE(e.code() == std::future_errc::promise_already_satisfied);
      } catch(...) {
         CATCH_REQUIRE(false);
      }
      CATCH_REQUIRE(future.get() == 42);
      CATCH_REQUIRE(!future.valid());
      CATCH_REQUIRE(!future.is_ready());
   }

   CATCH_SECTION("future-move-only-set")
   {
      Promise<std::unique_ptr<int>> promise;
      auto future = promise.get_future();
      CATCH_REQUIRE(future.valid());
      CATCH_REQUIRE(future.is_ready() == false);
      promise.set_value(std::make_unique<int>(42));
      CATCH_REQUIRE(future.is_ready());
      CATCH_REQUIRE(!future.has_exception());
      CATCH_REQUIRE(future.valid());
      std::unique_ptr<int> value = future.get();
      CATCH_REQUIRE(!future.valid());
      CATCH_REQUIRE(!future.is_ready());
      CATCH_REQUIRE(value != nullptr);
      CATCH_REQUIRE(*value == 42);
   }

   CATCH_SECTION("packaged-task-move-only-set")
   {
      auto work = PackagedTask<std::unique_ptr<int>, int>(
          [](int value) { return std::make_unique<int>(value); });

      auto future = work.get_future();
      CATCH_REQUIRE(future.valid());
      CATCH_REQUIRE(future.is_ready() == false);
      work(42);
      CATCH_REQUIRE(future.valid());
      CATCH_REQUIRE(future.is_ready() == true);
      CATCH_REQUIRE(!future.has_exception());
      std::unique_ptr<int> value = future.get();
      CATCH_REQUIRE(!future.valid());
      CATCH_REQUIRE(!future.is_ready());
      CATCH_REQUIRE(value != nullptr);
      CATCH_REQUIRE(*value == 42);
   }

   CATCH_SECTION("threaded-w-custom-allocator-asio")
   {
      constexpr std::size_t num_threads = 4;
      boost::asio::io_context io_context;
      auto wg = boost::asio::make_work_guard(io_context);
      std::vector<std::thread> pool;
      pool.reserve(num_threads);
      for(std::size_t i = 0; i < num_threads; ++i)
         pool.emplace_back([&io_context]() { io_context.run(); });

      run_test_async_(asio::ExecutionBroker{io_context});

      wg.reset();
      for(auto& thread : pool) thread.join();
   }

   CATCH_SECTION("threaded-w-custom-allocator-thread-pool")
   {
      constexpr std::size_t num_threads = 4;
      ThreadPool pool{1, 2, 1};
      TimerManager timer_manager{pool.get_executor()};

      run_test_async_(ExecutionBroker{timer_manager});

      timer_manager.dispose();
      pool.dispose();
   }
}

} // namespace niggly::async
