
#include "niggly/utils.hpp"

namespace niggly::example
{
static void tick_tock_example() noexcept
{
   const auto now = tick();
   for(auto i = 0; i < 1000; ++i) {
      // Do some stuff
   }
   const double elapsed_seconds = tock(now);
}

} // namespace niggly::example
