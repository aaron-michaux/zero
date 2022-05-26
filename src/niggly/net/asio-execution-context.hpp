
#pragma once

//#include "execution-broker.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <memory>
#include <thread>
#include <vector>

namespace niggly::net {

/**
 * Type erase the underlying boost::asio::io_context
 */
class AsioExecutionContext {
private:
  mutable boost::asio::io_context io_context_;
  std::size_t size_;
  std::vector<std::thread> pool_;

public:
  AsioExecutionContext(std::size_t thread_pool_size = 0)
      : size_{thread_pool_size == 0 ? std::thread::hardware_concurrency() : thread_pool_size} {
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
  auto get_executor() const { return io_context_.get_executor(); }

  /** @brief Direct access to the underlying io_context */
  boost::asio::io_context& io_context() { return io_context_; }
};

} // namespace niggly::net
