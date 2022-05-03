
#include "logging.hpp"

#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <cassert>

namespace niggly::logging
{
/// @private
static std::shared_ptr<spdlog::logger> instance;

/// @private
static std::once_flag flag;

/// @private
static void init_logger(std::shared_ptr<spdlog::logger> instance_)
{
   assert(instance_);
   instance = instance_;
}

/**
 * @ingroup logging
 * @brief lazily initializes and returns the logger instance.
 */
spdlog::logger& debug_logger()
{
   if(!instance) {
      std::call_once(flag, []() {
         init_logger(spdlog::stdout_color_mt("terminal"));
#ifdef DEBUG_BUILD
         instance->set_level(spdlog::level::trace);
#else
         instance->set_level(spdlog::level::info);
#endif
         instance->set_pattern("[%Y-%m-%d %T] [%^%l%$] %v");
      });
   }

   assert(instance);
   return *instance;
}

} // namespace niggly::logging
