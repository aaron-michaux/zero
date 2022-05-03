
#pragma once

#include "spdlog/spdlog.h"

/**
 * @defgroup logging Logging
 * @ingroup niggly-utils
 *
 * @see https://github.com/gabime/spdlog
 *
 * A convenience wrapper around `spdlog`. The
 * logger instance is held as a `static shared_ptr` in `logging.cpp`.
 *
 * Only supports 1 default logger, that outputs to stdout/stderr.
 * (That's all most people need.) The logger is lazily initialized
 * by the `niggly::logger()` function.
 */

namespace niggly::logging
{
using logger_type = spdlog::logger;

logger_type& debug_logger();

enum class LogLevel : int { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };

namespace detail
{
   template<std::size_t N> struct static_string
   {
      char str[N]{};
      constexpr static_string(const char (&s)[N])
      {
         for(std::size_t i = 0; i < N; ++i) str[i] = s[i];
      }
   };

   template<static_string s> struct format_string
   {
      static constexpr const char* string = s.str;
   };

   /**
    * For the logging macros  a constexpr string type through to the underlying call to
    * fmt::format
    */
   template<static_string s> constexpr auto operator""_cfmt() { return format_string<s>{}; }

   template<typename F, typename... Args>
   inline void log_internal_(LogLevel level, logger_type& logger, F, Args&&... args)
   {
      switch(level) {
      case LogLevel::TRACE: logger.trace(F::string, std::forward<Args>(args)...); break;
      case LogLevel::DEBUG: logger.debug(F::string, std::forward<Args>(args)...); break;
      case LogLevel::INFO: logger.info(F::string, std::forward<Args>(args)...); break;
      case LogLevel::WARN: logger.warn(F::string, std::forward<Args>(args)...); break;
      case LogLevel::ERROR: logger.error(F::string, std::forward<Args>(args)...); break;
      case LogLevel::FATAL:
         logger.critical(F::string, std::forward<Args>(args)...);
         std::terminate();
      }
   }
} // namespace detail

template<typename... Args> inline void log_trace(logger_type& logger, Args&&... args)
{
   detail::log_internal_(LogLevel::TRACE, logger, std::forward<Args>(args)...);
}

template<typename... Args> inline void log_debug(logger_type& logger, Args&&... args)
{
   detail::log_internal_(LogLevel::DEBUG, logger, std::forward<Args>(args)...);
}

template<typename... Args> inline void log_info(logger_type& logger, Args&&... args)
{
   detail::log_internal_(LogLevel::INFO, logger, std::forward<Args>(args)...);
}

template<typename... Args> inline void log_warn(logger_type& logger, Args&&... args)
{
   detail::log_internal_(LogLevel::WARN, logger, std::forward<Args>(args)...);
}

template<typename... Args> inline void log_error(logger_type& logger, Args&&... args)
{
   detail::log_internal_(LogLevel::ERROR, logger, std::forward<Args>(args)...);
}

template<typename... Args> inline void log_fatal(logger_type& logger, Args&&... args)
{
   detail::log_internal_(LogLevel::FATAL, logger, std::forward<Args>(args)...);
}

} // namespace niggly::logging
