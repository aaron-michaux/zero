
#include "niggly/portable/platform.hpp"

#include "asio-execution-context.hpp"

namespace niggly {

template <> struct AsioPlatform::Pimpl {

  friend AsioPlatform make_asio_platform(boost::asio::io_context& io_context,
                                         std::size_t thread_pool_size);
};

template <> AsioPlatform::Platform() : pimpl_{std::make_unique<AsioPlatform::Pimpl>()} {};

// template <> AsioPlatform::SteadyTimer make_steady_timer() {

// template <> AsioPlatform::ExecutionContext& AsioPlatform::execution_context() {
//   //
// }

AsioPlatform make_asio_platform(boost::asio::io_context& io_context, std::size_t thread_pool_size) {
  return {};
}

} // namespace niggly
