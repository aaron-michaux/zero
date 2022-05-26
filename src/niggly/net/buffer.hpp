
#pragma once

#include <vector>

namespace niggly::net {
/**
 * @todo We need a movable allocator-aware type to encapsulate a sequence of buffers
 */
using BufferType = std::vector<char>;
} // namespace niggly::net
