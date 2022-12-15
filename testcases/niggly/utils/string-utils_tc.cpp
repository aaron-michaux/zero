
#include "stdinc.hpp"

#include "niggly/utils/string-utils.hpp"

#include <catch2/catch_all.hpp>

namespace niggly::tests {

CATCH_TEST_CASE("StrUtils", "[str-utils]") {
  CATCH_SECTION("str-utils") {

    CATCH_REQUIRE(explode("", '/').size() == 1);

    {
      const auto parts = explode("one/two//three/", '/');
      CATCH_REQUIRE(parts.size() == 5);
      CATCH_REQUIRE(parts[0] == "one");
      CATCH_REQUIRE(parts[1] == "two");
      CATCH_REQUIRE(parts[2] == "");
      CATCH_REQUIRE(parts[3] == "three");
      CATCH_REQUIRE(parts[4] == "");
    }
  }
}
} // namespace niggly::tests
