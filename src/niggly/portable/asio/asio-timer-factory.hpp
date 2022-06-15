
#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <functional>

namespace niggly::net {
/**
 * @brief Type erase the creation of steady-timer objects
 */
inline std::function<boost::asio::steady_timer()>
make_steady_timer_factory(boost::asio::io_context& io_context) {
  return [&io_context]() { return boost::asio::steady_timer{io_context}; };
}

} // namespace niggly::net
