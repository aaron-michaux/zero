
#pragma once

#include "error-codes.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace niggly
{
// ------------------------------------------------------- file-get/put-contents

error_code file_get_contents(const std::string_view fname, std::string& out);
error_code file_get_contents(const std::string_view fname, std::vector<char>& out);
std::string file_get_contents(const std::string_view fname);

error_code file_put_contents(const std::string_view fname, const std::string_view dat);
error_code file_put_contents(const std::string_view fname, const std::vector<char>& dat);

// ----------------------------------------------------------- is-file/directory

bool is_regular_file(const std::string_view filename);
bool is_directory(const std::string_view filename);

// --------------------------------------------------- basename/dirname/file_ext

std::string absolute_path(const std::string_view filename);
std::string basename(const std::string_view filename, const bool strip_extension = false);
std::string dirname(const std::string_view filename);
std::string file_ext(const std::string_view filename); // like ".png"
std::string extensionless(const std::string_view filename);

// ----------------------------------------------------------------------- mkdir

bool mkdir(const std::string_view dname);
bool mkdir_p(const std::string_view dname);

} // namespace niggly
