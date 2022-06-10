
#pragma once

#include <chrono>
#include <functional>
#include <iostream>

/**
 * @defgroup tick-tock Timing Functions
 * @ingroup niggly-utils
 *
 *
 */

namespace niggly {
/// @brief Type used by the `tick()` and `tock()` functions.
using ticktock_type = std::chrono::time_point<std::chrono::steady_clock>;

// ------------------------------------------------------------------------ tick
/**
 * @ingroup tick-tock
 * @brief Reads a `std::chrono::steady_clock`.
 *
 * @include tick-tock_ex.cpp
 */
inline ticktock_type tick() { return std::chrono::steady_clock::now(); }

// ------------------------------------------------------------------------ tock
/**
 * @ingroup tick-tock
 * @brief Returns the elapsed seconds -- as a `double` -- since `whence`.
 * @see niggly::tick() for an example.
 */
inline double tock(const ticktock_type& whence) {
  using ss = std::chrono::duration<double, std::ratio<1, 1>>;
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<ss>(now - whence).count();
}

// ------------------------------------------------------------------ time thunk
/**
 * @ingroup tick-tock
 * @brief Executes the passed thunk, returning the wall-clock time taken in
 *        seconds
 *
 * @include time-thunk_ex.cpp
 */
template <typename F> constexpr double time_thunk(F&& f) {
  auto now = tick();
  f();
  return tock(now);
}

} // namespace niggly
