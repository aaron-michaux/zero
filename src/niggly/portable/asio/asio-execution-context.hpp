
#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <memory>
#include <thread>
#include <vector>

namespace niggly::net {

/**
 * @defgroup niggle-asio-beast Niggly Asio/Beast
 *
 * We use Asio for two things: an execution context, and managing timers.
 * Beast is used to manage websockets. These three dependencies are not
 * portable to WebAssembly.
 *
 * The type erasure
 */

/**
 * @brief Type erase the underlying boost::asio::io_context
 */
class AsioExecutionContext {
private:
  boost::asio::io_context& io_context_;
  std::size_t size_;
  std::vector<std::thread> pool_;

public:
  using ExecutorType = boost::asio::io_context::executor_type;
  using SteadyTimerType = boost::asio::steady_timer;

  AsioExecutionContext(boost::asio::io_context& io_context, std::size_t thread_pool_size = 0)
      : io_context_{io_context}, size_{thread_pool_size == 0 ? std::thread::hardware_concurrency()
                                                             : thread_pool_size} {
    pool_.reserve(thread_pool_size);
  }

  ~AsioExecutionContext() {
    for (auto& thread : pool_)
      thread.join();
  }

  /** @brief Run the pool */
  void run() {
    assert(!is_running());
    for (std::size_t i = 0; i < size_; ++i)
      pool_.emplace_back([this]() { io_context_.run(); });
  }

  /** @brief true iff the execution context is running */
  bool is_running() const noexcept { return pool_.size() > 0; }

  /** @brief Number of threads executing io requests in parallel */
  std::size_t size() const noexcept { return size_; }

  /** @brief Return the executor for running jobs on the server pool */
  ExecutorType get_executor() const { return io_context_.get_executor(); }

  /** @brief Create a new steady timer bound to this execution context */
  SteadyTimerType make_steady_timer() const { return SteadyTimerType{io_context_}; }

  /** @brief Direct access to the underlying io_context */
  // boost::asio::io_context& io_context() { return io_context_; }
};

} // namespace niggly::net
