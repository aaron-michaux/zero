
#include "niggly/portable/platform.hpp"

#include "asio-execution-context.hpp"

namespace niggly {

template <> struct AsioPlatform::Pimpl {};

template <> AsioPlatform::Platform() : pimpl_{std::make_unique<AsioPlatform::Pimpl>()} {};
} // namespace niggly
