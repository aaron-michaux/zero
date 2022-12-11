
#pragma once

#include <boost/endian/conversion.hpp>

#include <vector>
#include <span>
#include <string_view>

namespace niggly::net {
/**
 * @todo We need a movable allocator-aware type to encapsulate a sequence of buffers
 */
using BufferType = std::vector<std::byte>;

inline std::span<const std::byte> to_span_bytes(const BufferType& buffer) {
  return {buffer.data(), buffer.data() + buffer.size()};
}

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

inline BufferType& operator<<(BufferType& buffer, std::string_view ss) {
  const auto offset = buffer.size();
  buffer.resize(offset + ss.size());
  std::memcpy(&buffer[offset], ss.data(), ss.size());
  return buffer;
}

// inline BufferType& operator<<(BufferType& buffer, uint32_t value) {
//   const auto offset = buffer.size();
//   buffer.resize(offset + sizeof(uint32_t));
//   const auto network = boost::endian::native_to_big(value);
//   std::memcpy(&buffer[offset], &network, sizeof(uint32_t));
//   return buffer;
// }

} // namespace niggly::net
