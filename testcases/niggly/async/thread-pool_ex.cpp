
#include "niggly/async/timer-manager.hpp"

#include "stdinc.hpp"

#include <catch2/catch.hpp>

#include <boost/asio.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/execution.hpp>
#include <boost/asio/static_thread_pool.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <numeric>
#include <queue>
#include <vector>

namespace niggly::async
{

// ------------------------------------------------------------------- TEST_CASE
//
CATCH_TEST_CASE("ThreadPoolTimers", "[thread-pool-timers]")
{
   CATCH_SECTION("thread-pool-timers") { TRACE("HERE"); }
}

} // namespace niggly::async
