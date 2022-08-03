
#pragma once

#include "asio/asio-execution-context.hpp"

/**
 * @defgroup portable Portable
 */

/**
 * @brief The "platform" for portability
 *
 * The idealized target platforms are:
 * + WebAssembly, MacOS, Windows
 * + llvm, gcc, msvc
 *
 * Asio/Beast does not work on WebAssembly, and is used for
 * + An Execution Context (thread pool)
 * + Running timers (steady timer)
 * + Websockets
 *
 * Platform serves as a step towards portability
 */

namespace niggly {

template <typename ExecutionContext, typename SteadyTimer> class Platform {
private:
  struct Pimpl;
  std::unique_ptr<Pimpl> pimpl_;

public:
  using SteadyTimerType = SteadyTimer;
  using ExecutionContextText = ExecutionContext;

  Platform();
  // ExecutionContext& execution_context();
  // SteadyTimer make_steady_timer();
};

using AsioPlatform = Platform<net::AsioExecutionContext, boost::asio::steady_timer>;

AsioPlatform make_asio_platform(boost::asio::io_context& io_context,
                                std::size_t thread_pool_size = 0);

} // namespace niggly
