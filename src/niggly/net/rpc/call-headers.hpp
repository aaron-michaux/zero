
#pragma once

#include "status.hpp"

#include "niggly/net/buffer.hpp"

#include <cstdint>

namespace niggly::net::detail {

/**
 * @brief The header is sent in network byte order
 */
struct RequestEnvelopeHeader {
  bool is_request{true};              //!< It's either a request or a response; should be true
  uint64_t request_id{0};             //!< So the client can track responses
  uint32_t call_id{0};                //!< The call handler to match
  uint32_t deadline_millis{0};        //!< Milliseconds to respond to call; 0 means no deadline
  std::span<const std::byte> payload; //!< The envelope parameters
};

/**
 * @return false iff there's an error encoding the data
 */
bool encode_request_header(BufferType& buffer, uint64_t request_id, uint32_t call_id,
                           uint32_t deadline_millis);

/**
 * @brief Decode the buffer `data`+`size` into `header`
 */
bool decode(RequestEnvelopeHeader& header, std::span<const std::byte> payload);

struct ResponseEnvelopeHeader {
  bool is_request{false};             //!< It's either a request or a response; should be false
  uint64_t request_id{0};             //!< So the client can track responses
  Status status;                      //!< The response status from the server
  std::span<const std::byte> payload; //!< The envelope parameters
};

/**
 * @return false iff there's an error encoding the data
 */
bool encode_response_header(BufferType& buffer, uint64_t request_id, const Status& status);

/**
 * @brief Decode the buffer `data`+`size` into `header`
 */
bool decode(ResponseEnvelopeHeader& header, std::span<const std::byte> payload);

} // namespace niggly::net::detail
