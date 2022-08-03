
#pragma once

#include <vector>
#include <span>
#include <string_view>

namespace niggly::net {
/**
 * @todo We need a movable allocator-aware type to encapsulate a sequence of buffers
 */
using BufferType = std::vector<std::byte>;

inline BufferType make_send_buffer(std::string_view ss) {
  const std::byte* data = reinterpret_cast<const std::byte*>(ss.data());
  return BufferType{data, data + ss.size()};
}

inline BufferType make_send_buffer(std::span<std::byte> ss) {
  return BufferType{cbegin(ss), cend(ss)};
}

inline BufferType make_send_buffer(std::span<const std::byte> ss) {
  return BufferType{cbegin(ss), cend(ss)};
}

} // namespace niggly::net
