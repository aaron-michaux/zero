
#pragma once

#include "spdlog/spdlog.h"

#include "threads.hpp"

#include <cstdint>
#include <random>

namespace niggly
{
/**
 * Service -- i.e., Global State -- for the `niggly` library
 */
struct Service
{
   spdlog::logger& logger;
   std::minstd_rand& random_engine;
   ThreadPool& executor;
};

namespace detail
{
   Service make_default_service_();
} // namespace detail

inline Service& get_default_service()
{
   static Service service_ = detail::make_default_service_();
   return service_;
}

#define default_logger() service.logger

inline auto& get_default_logger() { return get_default_service().logger; }
inline auto& get_default_random_engine() { return get_default_service().random_engine; }
inline auto& get_default_executor() { return get_default_service().executor; }

} // namespace niggly
