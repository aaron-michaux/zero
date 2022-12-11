
#pragma once

#include "base/format.hpp"
#include "base/sso-string.hpp"

#include <cassert>
#include <cfloat>
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @defgroup niggly-strings Strings
 * @ingroup niggly-utils
 */
namespace niggly {
// ---------------------------------------------------------------------- Indent

string indent(const string& s, int level);

// -------------------------------------------------------------- To Upper/Lower
/**
 * @ingroup niggly-strings
 * @brief converts `s` to uppercase inplace.
 */
template <class Range>
requires ranges::range<Range>
constexpr Range to_upper(Range r) {
  for (auto& c : r)
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return r;
}

/**
 * @ingroup niggly-strings
 * @brief converts `s` to lowercase inplace.
 */
template <class Range>
requires ranges::range<Range>
constexpr Range to_lower(Range r) {
  for (auto& c : r)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return r;
}

/**
 * @ingroup niggly-strings
 * @brief copyies and converts `s` to uppercase.
 */
template <typename string_type> constexpr string_type to_upper_copy(const string_type& s) {
  string_type t = s;
  to_upper(t);
  return t;
}

/**
 * @ingroup niggly-strings
 * @brief copyies and converts `s` to lowercase
 */
template <typename string_type> constexpr string_type to_lower_copy(const string_type& s) {
  string_type t = s;
  to_lower(t);
  return t;
}

// --------------------------------------------------------------------- explode

template <typename Container = std::vector<std::string_view>>
inline Container explode(std::string_view input, char delim) {
  Container out;
  auto start = std::begin(input);

  std::string_view::size_type pos0 = 0;
  while (true) {
    auto pos1 = input.find_first_of(delim, pos0);
    auto len = (pos1 == std::string_view::npos) ? input.size() - pos0 : pos1 - pos0;
    out.emplace_back(start + pos0, len);
    if (pos1 == std::string_view::npos) {
      break;
    }
    pos0 = pos1 + 1;
  }

  return out;
}

// ------------------------------------------------------------------------ Trim

string& ltrim(std::string& s);
string& rtrim(std::string& s);
string& trim(std::string& s);
string trim_copy(const std::string& s);

// Helper function
namespace detail {
/**
 * @ingroup niggly-strings
 * @private
 */
template <typename T> sso23::string format_(const char* fmt, const T& v) {
  constexpr int k_size = 23;
  char buffer[k_size];
  auto written = snprintf(buffer, k_size, fmt, v);
  assert(written >= 0);
  if (written < k_size - 1)
    return sso23::string(buffer);
  // We never do this in our str(...) functions
  unsigned buflen = unsigned(written + 1);
  std::unique_ptr<char[]> b2(new char[buflen]);
  snprintf(b2.get(), buflen, fmt, v);
  return sso23::string(b2.get());
}

} // namespace detail

// ------------------------------------------------------- string shim functions

/**
 * @ingroup niggly-strings
 * @brief A string shim... not absolutely efficient, but simple to use.
 */
template <typename T, typename string_type = sso23::string>
requires std::is_fundamental<T>::value string_type str(const T& v) {
  using U = typename std::decay<T>::type;

  // Missing: char16_t, char32_t, wchar_t

  if constexpr (std::is_same<U, char*>::value) {
    return string_type(v);
  } else if constexpr (std::is_same<U, const char*>::value) {
    return string_type(v);
  } else if constexpr (std::is_pointer<U>::value) {
    return detail::format_<const void*>("%p", static_cast<const void*>(v));
  } else if constexpr (std::is_same<U, bool>::value) { // boolean
    return v ? string_type{"true"} : string_type{"false"};
  } else if constexpr (std::is_same<U, signed char>::value) { // Char types
    return detail::format_<U>("%d", v);
  } else if constexpr (std::is_same<U, unsigned char>::value) {
    return detail::format_<U>("%u", v);
  } else if constexpr (std::is_same<U, int8_t>::value) { // Char types
    return detail::format_<U>("%d", v);
  } else if constexpr (std::is_same<U, uint8_t>::value) {
    return detail::format_<U>("%u", v);
  } else if constexpr (std::is_same<U, char>::value) { // Char types
    return string_type(1, v);
  } else if constexpr (std::is_same<U, char8_t>::value) {
    return string_type(1, char(v));
  } else if constexpr (std::is_same<U, short int>::value) { // integral types
    return detail::format_<U>("%d", v);
  } else if constexpr (std::is_same<U, unsigned short int>::value) {
    return detail::format_<U>("%u", v);
  } else if constexpr (std::is_same<U, int>::value) {
    return detail::format_<U>("%d", v);
  } else if constexpr (std::is_same<U, unsigned int>::value) {
    return detail::format_<U>("%u", v);
  } else if constexpr (std::is_same<U, long int>::value) {
    return detail::format_<U>("%ld", v);
  } else if constexpr (std::is_same<U, unsigned long int>::value) {
    return detail::format_<U>("%lu", v);
  } else if constexpr (std::is_same<U, long long int>::value) {
    return detail::format_<U>("%lld", v);
  } else if constexpr (std::is_same<U, unsigned long long int>::value) {
    return detail::format_<U>("%llu", v);
  } else if constexpr (std::is_floating_point<U>::value) { // Floating point
    return detail::format_<double>("%f", static_cast<double>(v));
  } else {
    FATAL("not implemented");
  }
}

/**
 * @ingroup niggly-strings
 * @brief Overload for `str(...)` that efficiently handles references.
 */
constexpr const sso23::string& str(const sso23::string& s) { return s; }

/**
 * @ingroup niggly-strings
 * @brief Overload for `str(...)` that efficiently handles rvalue refs.
 */
constexpr const sso23::string str(sso23::string&& s) { return std::move(s); }

/**
 * @ingroup niggly-strings
 * @brief Prints `v` as a decimal floating point with maximum precision.
 */
inline sso23::string str_precise(double v) {
  constexpr int k_size = 256;
  constexpr int dec_dig = std::numeric_limits<double>::max_digits10;
  char buffer[k_size];
  auto written = snprintf(buffer, k_size, "%.*e", dec_dig, v);
  assert(written >= 0);
  if (written < k_size - 1)
    return sso23::string(buffer);
  // We never do this in our str(...) functions
  auto buflen = size_t(written + 1);
  std::unique_ptr<char[]> b2(new char[buflen]);
  snprintf(b2.get(), buflen, "%.*e", dec_dig, v);
  return sso23::string(b2.get());
}

// ------------------------------------------------- Pretty Printing binary data

/**
 * @ingroup niggly-strings
 * @brief The raw hex string of `data`, as if via the shell command `xxd`
 */
sso23::string str(const void* data, size_t sz);
sso23::string str(std::span<std::byte> data);
sso23::string str(std::span<const std::byte> data);

/**
 * @ingroup niggly-strings
 * @brief Print the bit-pattern of `o`
 */
template <typename T> constexpr sso23::string to_bitstring(const T& o) {
  const int n_bits = sizeof(o) * 8;
  const int size = n_bits           // the bits themselves
                   + sizeof(o)      // every group of eight bits has a hyphen in the middle
                   + sizeof(o) - 1; // space between every group of eights
  const uint8_t* begin = reinterpret_cast<const uint8_t*>(&o);
  const uint8_t* end = begin + sizeof(o);

  auto write_byte = [](const int byte, sso23::string& out, int& pos) {
    for (auto i = 7; i >= 0; --i) {
      if (i == 3)
        out[pos++] = '-';
      const bool bit = ((int(0x01) << i) & byte) >> i;
      out[pos++] = bit ? '1' : '0';
    }
  };

  sso23::string out(size, ' ');
  int pos = 0; // write position
  for (auto ii = begin; ii != end; ++ii) {
    if (ii != begin)
      pos++; // skip the space
    write_byte(*ii, out, pos);
  }

  assert(pos == size);
  return out;
}

// ------------------------------------------------------------ Terminal Colours

/// @ingroup niggly-strings
constexpr const char* ANSI_COLOUR_RED = "\x1b[31m";
/// @ingroup niggly-strings
constexpr const char* ANSI_COLOUR_GREEN = "\x1b[32m";
/// @ingroup niggly-strings
constexpr const char* ANSI_COLOUR_YELLOW = "\x1b[33m";
/// @ingroup niggly-strings
constexpr const char* ANSI_COLOUR_BLUE = "\x1b[34m";
/// @ingroup niggly-strings
constexpr const char* ANSI_COLOUR_MAGENTA = "\x1b[35m";
/// @ingroup niggly-strings
constexpr const char* ANSI_COLOUR_CYAN = "\x1b[36m";
/// @ingroup niggly-strings
constexpr const char* ANSI_COLOUR_GREY = "\x1b[37m";

/// @ingroup niggly-strings
constexpr const char* ANSI_COLOUR_RED_BG = "\x1b[41m";
/// @ingroup niggly-strings
constexpr const char* ANSI_COLOUR_GREEN_BG = "\x1b[42m";
/// @ingroup niggly-strings
constexpr const char* ANSI_COLOUR_YELLOW_BG = "\x1b[43m";
/// @ingroup niggly-strings
constexpr const char* ANSI_COLOUR_BLUE_BG = "\x1b[44m";

/// @ingroup niggly-strings
constexpr const char* ANSI_COLOUR_RESET = "\x1b[0m";

} // namespace niggly
