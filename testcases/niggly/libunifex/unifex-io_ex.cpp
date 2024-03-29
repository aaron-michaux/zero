
#include "stdinc.hpp"

#include <catch2/catch_all.hpp>

#include <unifex/config.hpp>

#include <unifex/inplace_stop_token.hpp>
#include <unifex/just.hpp>
#include <unifex/let_value_with.hpp>
#include <unifex/linux/io_uring_context.hpp>
#include <unifex/scheduler_concepts.hpp>
#include <unifex/scope_guard.hpp>
#include <unifex/sequence.hpp>
#include <unifex/sync_wait.hpp>
#include <unifex/then.hpp>
#include <unifex/when_all.hpp>
#include <unifex/with_query_value.hpp>
#include <unifex/just_from.hpp>

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

// using namespace unifex;
// using namespace unifex::linuxos;
using namespace std::chrono_literals;

using io_uring_context = unifex::linuxos::io_uring_context;

template <typename S> auto discard_value(S&& s) {
  return unifex::then(std::move(s), [](auto&&...) noexcept {});
}

static constexpr unsigned char data[6] = {'h', 'e', 'l', 'l', 'o', '\n'};

// This could be made generic across any scheduler that supports the
// async_write_only_file() CPO.
auto write_new_file(io_uring_context::scheduler s, std::string_view path) {
  using namespace unifex;
  return let_value_with(
      [s, path]() {
        // Call the 'open_file_write_only' CPO with the scheduler.
        // This will return a file object that satisfies an
        // async-write-file concept.
        return unifex::open_file_write_only(s, path);
      },
      [](io_uring_context::async_write_only_file& file) {
        const auto buffer = as_bytes(span{data});
        // Start 8 concurrent writes to the file at different offsets.
        return discard_value(when_all(
            // Calls the 'async_write_some_at()' CPO on the file object
            // returned from 'open_file_write_only()'.
            async_write_some_at(file, 0, buffer),
            async_write_some_at(file, 1 * buffer.size(), buffer),
            async_write_some_at(file, 2 * buffer.size(), buffer),
            async_write_some_at(file, 3 * buffer.size(), buffer),
            async_write_some_at(file, 4 * buffer.size(), buffer),
            async_write_some_at(file, 5 * buffer.size(), buffer),
            async_write_some_at(file, 6 * buffer.size(), buffer),
            async_write_some_at(file, 7 * buffer.size(), buffer)));
      });
}

auto read_file(io_uring_context::scheduler s, std::string_view path) {
  return unifex::let_value_with(
      [s, path]() { return unifex::open_file_read_only(s, path); },
      [buffer = std::vector<char>{}](auto& file) mutable {
        buffer.resize(100);
        return unifex::then(
            unifex::async_read_some_at(
                file, 0, unifex::as_writable_bytes(unifex::span{buffer.data(), buffer.size() - 1})),
            [&](ssize_t bytesRead) {
              std::printf("read %zi bytes\n", bytesRead);
              buffer[bytesRead] = '\0';
              std::printf("contents: %s\n", buffer.data());
            });
      });
}

void testzappy() {
  using namespace unifex;
  io_uring_context ctx;

  inplace_stop_source stopSource;
  std::thread t{[&] { ctx.run(stopSource.get_token()); }};
  scope_guard stopOnExit = [&]() noexcept {
    stopSource.request_stop();
    t.join();
  };

  auto scheduler = ctx.get_scheduler();

  try {
    {
      auto startTime = std::chrono::steady_clock::now();
      inplace_stop_source timerStopSource;
      sync_wait(with_query_value(when_all(then(schedule_at(scheduler, now(scheduler) + 1s),
                                               []() { std::printf("timer 1 completed (1s)\n"); }),
                                          then(schedule_at(scheduler, now(scheduler) + 2s),
                                               []() { std::printf("timer 2 completed (2s)\n"); }),
                                          then(schedule_at(scheduler, now(scheduler) + 1500ms),
                                               [&]() {
                                                 std::printf(
                                                     "timer 3 completed (1.5s) cancelling\n");
                                                 timerStopSource.request_stop();
                                               })),
                                 get_stop_token, timerStopSource.get_token()));
      auto endTime = std::chrono::steady_clock::now();

      std::printf(
          "completed in %i ms\n",
          static_cast<int>(
              std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count()));
    }

    sync_wait(sequence(just_from([] { std::printf("writing file\n"); }),                //
                       write_new_file(scheduler, "test.txt"),                           //
                       just_from([] { std::printf("write completed, waiting 1s\n"); }), //
                       just_from([] { std::printf("reading file concurrently\n"); }),   //
                       read_file(scheduler, "test.txt")));                              //

  } catch (const std::exception& ex) {
    std::printf("error: %s\n", ex.what());
  }
}

static void example_unifex_hello_world() {
  namespace ex = unifex;
  using namespace niggly;

  ex::scheduler auto sch = ex::thread_pool.scheduler();
  ex::sender auto begin = ex::schedule(sch);
  ex::sender auto hi = ex::then(begin, [] {
    std::cout << "Hello world! Have an int.";
    return 13;
  });
  ex::sender auto add_42 = ex::then(hi, [](int arg) { return arg + 42; });

  auto [i] = ex::this_thread::sync_wait(add_42).value();
  TRACE("i = {}", i);
}

namespace niggly::unifex::test {

// -------------------------------------------------------------------
//
CATCH_TEST_CASE("UnifexIO", "[unifex-io]") {
  CATCH_SECTION("unifex-io") { example_unifex_hello_world(); }
}

} // namespace niggly::unifex::test
