
#pragma once

#include "sso-string.hpp"

#include <fmt/format.h>

namespace sso23 {
namespace detail {
constexpr std::size_t max_vformat_buffer_size_ = 256;

static thread_local inline std::vector<string::value_type> vformat_buffer_;
} // namespace detail

inline string vformat(fmt::string_view format_str, fmt::format_args args) {
  auto& buffer = detail::vformat_buffer_;
  fmt::vformat_to(std::back_inserter(buffer), format_str, args);
  auto result = string{buffer.data(), buffer.size()};
  if (buffer.size() > detail::max_vformat_buffer_size_) {
    // Would be cool to have a `string::buffer` type
    // that can just release it's internal buffer
    buffer.resize(detail::max_vformat_buffer_size_);
    buffer.shrink_to_fit();
  }
  return result;
}

template <typename... Args> inline string format(fmt::string_view format_str, const Args&... args) {
  return ::sso23::vformat(format_str, fmt::make_format_args(args...));
}

} // namespace sso23
