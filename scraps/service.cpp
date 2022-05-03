
#include "service.hpp"

#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace niggly::detail
{
static std::shared_ptr<spdlog::logger> make_default_logger_()
{
   auto logger = spdlog::stdout_color_mt("console");
   logger->set_level(spdlog::level::debug);
   logger->set_pattern("[%Y-%m-%d %T] [%^%l%$] %v");
   return logger;
}

Service make_default_service_()
{
   static std::shared_ptr<spdlog::logger> logger = make_default_logger_();
   static std::minstd_rand random_engine;
   static ThreadPool executor;
   return {*logger, random_engine, executor};
}

} // namespace niggly::detail
