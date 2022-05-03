
#include "niggly/utils.hpp"

namespace niggly::example
{
static void time_thunk_example() noexcept
{
   auto f = []() { return "after some long running task"; };

   const double elapsed_seconds = time_thunk(f);

   cout << format("f() took %g seconds", elapsed_seconds) << endl;
}

} // namespace niggly::example
