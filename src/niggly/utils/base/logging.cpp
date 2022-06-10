
#include "logging.hpp"

#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <cassert>

namespace niggly::logging {
/// @private
static std::shared_ptr<spdlog::logger> instance;

/// @private
static std::once_flag flag;

/// @private
static void init_logger(std::shared_ptr<spdlog::logger> instance_) {
  assert(instance_);
  instance = instance_;
}

/**
 * @ingroup logging
 * @brief lazily initializes and returns the logger instance.
 */
spdlog::logger& debug_logger() {
  if (!instance) {
    std::call_once(flag, []() {
      init_logger(spdlog::stdout_color_mt("terminal"));
      instance->set_pattern("[%Y-%m-%d %T] [%^%l%$] %v");

#ifdef DEBUG_BUILD
      instance->set_level(spdlog::level::trace);
#else
         instance->set_level(spdlog::level::warn);
#endif

      const char* env_variable = "LOG_LEVEL_OVERRIDE";
      const char* log_level = std::getenv(env_variable);
      if (log_level) {
        const auto level = spdlog::level::from_str(log_level);
        if (level == spdlog::level::off && log_level != std::string_view{"off"}) {
          instance->error("failed to set log level from environment variable {}={}", env_variable,
                          log_level);
        } else {
          instance->set_level(level);
        }
      }
    });
  }

  assert(instance);
  return *instance;
}

} // namespace niggly::logging
