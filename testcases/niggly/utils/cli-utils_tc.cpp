

#include "niggly/utils/cli-utils.hpp"

// TODO(remove)
#include "stdinc.hpp"

#include <catch2/catch_all.hpp>

namespace niggly::cli::tests {
CATCH_TEST_CASE("CliUtils", "[cli-utils]") {
  CATCH_SECTION("cli-utils") {
    std::vector<std::string> args = parse_cmd_args("exec-name 1 two three");
    std::vector<char*> argv_s;
    int argc = int(args.size());
    for (auto i = 0; i < argc; ++i)
      argv_s.push_back(args[i].data());
    char** argv = argv_s.data();

    {
      int i = 0;
      CATCH_REQUIRE(safe_arg_int(argc, argv, i) == 1);
      CATCH_REQUIRE(i == 1);
      CATCH_REQUIRE(safe_arg_str(argc, argv, i) == "two");
      CATCH_REQUIRE(i == 2);
      CATCH_REQUIRE(safe_arg_str(argc, argv, i) == "three");
      CATCH_REQUIRE(i == 3);
    }
  }
}
} // namespace niggly::cli::tests
