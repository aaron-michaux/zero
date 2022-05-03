
#include "niggly/utils/extra-algorithms.hpp"

namespace niggly::examples
{
static int g = 42;
constexpr int test2(int x) { return (x == 42) ? g : x; }
static_assert(!is_constexpr_friendly([] { test2(42); }));
static_assert(is_constexpr_friendly([] { test2(43); }));
} // namespace niggly::examples
