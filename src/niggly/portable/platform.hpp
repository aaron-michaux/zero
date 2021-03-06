
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
  ExecutionContext execution_context;

  SteadyTimer make_steady_timer();
};

using AsioPlatform = Platform<net::AsioExecutionContext, boost::asio::steady_timer>;

} // namespace niggly
