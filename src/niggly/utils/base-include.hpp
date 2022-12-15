
#pragma once

// ------------------------------------------------------------- Library defines

// Turn on 64bit files, stdio.h
#define _FILE_OFFSET_BITS 64

#ifndef __cplusplus
// --------------------------------------------------------------------------- C
#include <assert.h>
#include <stdio.h>

#else
// ------------------------------------------------------------------------- C++

// Contrib
#include <tl/expected.hpp>
#include <ofats/invocable.h>
#include <range/v3/all.hpp>

#define SPDLOG_FUNCTION __PRETTY_FUNCTION__
#define SPDLOG_FMT_EXTERNAL
#include "spdlog/spdlog.h"

// owner/observer pointer
#include "base/format.hpp"
#include "base/logging.hpp"
#include "base/pointer.hpp"
#include "base/sso-string.hpp"

#ifdef BENCHMARK_BUILD
#include <benchmark/benchmark.h>
#endif

#include <algorithm>
#include <compare>
#include <functional>
#include <memory>
#include <numeric>

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <span>

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace actions {
using namespace ranges::actions;
}

namespace views {
using namespace ranges::views;
}

namespace niggly {
// -----------------------------------------------------------------------------

using tl::expected;
using tl::make_unexpected;
using tl::unexpected;

using fmt::format;

using ofats::any_invocable;
using std::function;
using thunk_type = std::function<void()>;

using std::cerr;
using std::cout;
using std::endl;
using std::istream;
using std::ostream;
using std::stringstream;

using std::array;
using std::string;
using std::string_view;
using std::unordered_map;
using std::unordered_set;
using std::vector;

// Smart Pointers
using niggly::observer_ptr;
using niggly::owned_ptr;
using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::unique_ptr;

using std::begin;
using std::cbegin;
using std::cend;
using std::crbegin;
using std::crend;
using std::end;
using std::rbegin;
using std::rend;

using std::error_code;
using std::make_error_code;

using std::lock_guard; // May want to use a schedular/coroutine aware lock_guard

// NOTE:
//       "0"s is a string
//        1s is 1 second
// using namespace std::literals::string_literals;
using namespace std::literals::chrono_literals;
using namespace sso23::literals;

} // namespace niggly

namespace zero {
using namespace niggly;
}

#if defined __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#elif defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#endif

// ------------------------------------------------------------- Likely/unlikely

#if defined(__clang__) || defined(__GNUC__)
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (!!(x))
#define unlikely(x) (!!(x))
#endif

// --------------------------------------------------------------------- Logging

#ifdef DEBUG_BUILD
#define TRACE(fmt, ...)                                                                            \
  {                                                                                                \
    using namespace niggly::logging::detail;                                                       \
    ::niggly::logging::log_trace(logging::debug_logger(),                                          \
                                 "[\x1b[4m\x1b[97m{}:{}\x1b[0m] " fmt##_cfmt, __FILE__,            \
                                 __LINE__ __VA_OPT__(, ) __VA_ARGS__);                             \
  }

#define LOG_DEBUG(fmt, ...)                                                                        \
  {                                                                                                \
    using namespace niggly::logging::detail;                                                       \
    ::niggly::logging::log_debug(logging::debug_logger(),                                          \
                                 "[\x1b[4m\x1b[97m{}:{}\x1b[0m] " fmt##_cfmt, __FILE__,            \
                                 __LINE__ __VA_OPT__(, ) __VA_ARGS__);                             \
  }
#else
#define TRACE(fmt, ...)
#define LOG_DEBUG(fmt, ...)
#endif

#define INFO(fmt, ...)                                                                             \
  {                                                                                                \
    using namespace niggly::logging::detail;                                                       \
    ::niggly::logging::log_info(logging::debug_logger(),                                           \
                                "[\x1b[4m\x1b[97m{}:{}\x1b[0m] " fmt##_cfmt, __FILE__,             \
                                __LINE__ __VA_OPT__(, ) __VA_ARGS__);                              \
  }

#define WARN(fmt, ...)                                                                             \
  {                                                                                                \
    using namespace niggly::logging::detail;                                                       \
    ::niggly::logging::log_warn(logging::debug_logger(),                                           \
                                "[\x1b[4m\x1b[97m{}:{}\x1b[0m] " fmt##_cfmt, __FILE__,             \
                                __LINE__ __VA_OPT__(, ) __VA_ARGS__);                              \
  }

#define LOG_ERR(fmt, ...)                                                                          \
  {                                                                                                \
    using namespace niggly::logging::detail;                                                       \
    ::niggly::logging::log_error(logging::debug_logger(),                                          \
                                 "[\x1b[4m\x1b[97m{}:{}\x1b[0m] " fmt##_cfmt, __FILE__,            \
                                 __LINE__ __VA_OPT__(, ) __VA_ARGS__);                             \
  }

#define CRITICAL(fmt, ...)                                                                         \
  {                                                                                                \
    using namespace niggly::logging::detail;                                                       \
    ::niggly::logging::log_fatal(logging::debug_logger(),                                          \
                                 "[\x1b[4m\x1b[97m{}:{}\x1b[0m] " fmt##_cfmt, __FILE__,            \
                                 __LINE__ __VA_OPT__(, ) __VA_ARGS__);                             \
  }

#define FATAL(fmt, ...)                                                                            \
  {                                                                                                \
    using namespace niggly::logging::detail;                                                       \
    ::niggly::logging::log_fatal(logging::debug_logger(),                                          \
                                 "[\x1b[4m\x1b[97m{}:{}\x1b[0m] " fmt##_cfmt, __FILE__,            \
                                 __LINE__ __VA_OPT__(, ) __VA_ARGS__);                             \
  }

#ifdef Expects
#undef Expects
#endif
#ifdef NDEBUG
#define Expects(condition)
#else
#define Expects(condition)                                                                         \
  if (!likely(condition))                                                                          \
    FATAL("precondition failed: {}", #condition);
#endif

#ifdef Ensures
#undef Ensures
#endif
#ifdef NDEBUG
#define Ensures(condition)
#else
#define Ensures(condition)                                                                         \
  if (!likely(condition))                                                                          \
    FATAL("postcondition failed: {}", #condition);
#endif

#if defined __clang__
#pragma clang diagnostic pop
#elif defined __GNUC__
#pragma GCC diagnostic pop
#endif

#endif // #defined cplusplus
