
#include "base-include.hpp"

#include "string-utils.hpp"

#include <regex>

namespace niggly {
// ------------------------------------------------- Pretty Printing binary data

sso23::string str(const void* data, size_t sz) {
  // 0         1         2         3         4         5         6
  // 01234567890123456789012345678901234567890123456789012345678901234567
  // 00000000: 0a23 2050 7974 686f 6e2f 432b 2b20 4d75  .# Python/C++ Mu

  auto ptr = static_cast<const char*>(data);

  auto hexit = [](uint8_t c) -> char {
    if (c < 10)
      return char('0' + c);
    return char('a' + (c - 10));
  };

  const auto row_sz = 68;
  const auto n_rows = (sz % 16 == 0) ? (sz / 16) : (1 + sz / 16);
  auto out_sz = n_rows * row_sz;
  sso23::string out;
  out.resize(out_sz, ' ');

  char buffer[32];
  auto process_row = [&](const auto row_number) {
    const auto row_pos = row_number * row_sz;
    snprintf(buffer, 32, "%08zx:", size_t(row_number * 16));
    std::copy(&buffer[0], &buffer[0] + 9, &out[row_pos]);
    auto pos = row_pos + 9;
    auto ascii_pos = row_pos + 51;

    auto k = row_number * 16;
    for (auto i = k; i < k + 16 and i < sz; ++i) {
      if (i % 2 == 0)
        out[pos++] = ' ';
      const auto c = ptr[i];
      out[pos++] = hexit((c >> 4) & 0x0f);
      out[pos++] = hexit((c >> 0) & 0x0f);
      out[ascii_pos++] = std::isprint(c) ? c : '.';
    }
    out[row_pos + 67] = '\n';
  };

  for (std::decay_t<decltype(n_rows)> row = 0; row < n_rows; ++row)
    process_row(row);

  return out;
}

sso23::string str(std::span<std::byte> data) {
  return str(static_cast<const void*>(data.data()), data.size());
}

sso23::string str(std::span<const std::byte> data) {
  return str(static_cast<const void*>(data.data()), data.size());
}

// ---------------------------------------------------------------------- Indent
/**
 * @ingroup niggly-strings
 * @brief indents the passed string `s` by `level` spaces.
 */
std::string indent(const string& s, int level) {
  const std::string indent_s(size_t(level), char(' '));
  std::stringstream ss{""};
  std::istringstream input{s};
  for (string line; std::getline(input, line);)
    ss << indent_s << line << std::endl;
  string ret = ss.str();
  // Remove endl character if 's' doesn't have one
  if (s.size() > 0 && s[0] != '\n')
    ret.pop_back();
  return ret;
}

// ------------------------------------------------------------------------ Trim
/**
 * @ingroup niggly-strings
 * @brief Trims `s` from the left, inplace
 */
string& ltrim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
  return s;
}

/**
 * @ingroup niggly-strings
 * @brief Trims `s` from the right (ie, end), inplace
 */
string& rtrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(),
          s.end());
  return s;
}

/**
 * @ingroup niggly-strings
 * @brief Trims whitespace from the start and end of `s`, inplace.
 */
string& trim(std::string& s) {
  ltrim(s);
  rtrim(s);
  return s;
}

/**
 * @ingroup niggly-strings
 * @brief Trims whitespace from the start and end of `s`.
 */
string trim_copy(const std::string& s) {
  auto ret = s;
  trim(ret);
  return ret;
}

} // namespace niggly
