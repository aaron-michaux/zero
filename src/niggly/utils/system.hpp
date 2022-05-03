
#pragma once

#include <string>
#include <string_view>
#include <utility>

namespace niggly
{
std::pair<std::string, int> exec(std::string_view command);
}
