
#pragma once

#include <vector>

namespace niggly::net {
/**
 * @todo We need a movable allocator-aware type to encapsulate a sequence of buffers
 */
using BufferType = std::vector<char>;

inline BufferType make_send_buffer(std::string_view ss) { return BufferType{cbegin(ss), cend(ss)}; }

} // namespace niggly::net
