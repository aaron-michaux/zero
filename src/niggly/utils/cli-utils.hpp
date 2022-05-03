
#pragma once

#include <string>
#include <string_view>
#include <vector>

/**
 * @defgroup cli Command Line Utils
 * @ingroup niggly-utils
 *
 * The `niggly` method for parsing command-line arguments.
 *
 * @include cli-utils_ex.cpp
 */

namespace niggly::cli
{
std::string safe_arg_str(int argc, char** argv, int& i);
int safe_arg_int(int argc, char** argv, int& i);
double safe_arg_double(int argc, char** argv, int& i);

// ------------------------------------------------------------------ parse args

std::vector<std::string> parse_cmd_args(const std::string_view line);

} // namespace niggly::cli
