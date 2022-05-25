
// TODO(remove)
#include "stdinc.hpp"

#include <catch2/catch.hpp>

namespace niggly::tests
{

static std::vector<std::string_view> StrSplit(std::string_view input, char delim)
{
   std::vector<std::string_view> out;
   auto start = std::begin(input);

   std::string_view::size_type pos0 = 0;
   while(true) {
      auto pos1 = input.find_first_of(delim, pos0);
      auto len  = (pos1 == std::string_view::npos) ? input.size() - pos0 : pos1 - pos0;
      out.emplace_back(start + pos0, len);
      if(pos1 == std::string_view::npos) { break; }
      pos0 = pos1 + 1;
   }

   return out;
}

CATCH_TEST_CASE("StrUtils", "[str-utils]")
{
   CATCH_SECTION("str-utils")
   {
      auto parts = StrSplit("one/two//three/", '/');
      for(auto& s : parts) { TRACE("s = '{}'", s); }
   }
}
} // namespace niggly::tests
