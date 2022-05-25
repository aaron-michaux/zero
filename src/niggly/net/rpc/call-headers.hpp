
#pragma once

#include "status.hpp"

#include "niggly/net/websocket-session.hpp"

#include <cstdint>

namespace niggly::net::detail {

/**
 * @brief The header is sent in network byte order
 */
struct RequestEnvelopeHeader {
  bool is_request{true};       //!< It's either a request or a response; should be true
  uint64_t request_id{0};      //!< So the client can track responses
  uint32_t call_id{0};         //!< The call handler to match
  uint32_t deadline_millis{0}; //!< Milliseconds to respond to call; 0 means no deadline
  const void* data{nullptr};   //!< Pointer to the start of data
  std::size_t size{0};         //!< The amount of data
};

bool encode_request_header(WebsocketBufferType& buffer, uint64_t request_id, uint32_t call_id,
                           uint32_t deadline_millis);
bool decode(RequestEnvelopeHeader& header, const void* data, std::size_t size);

struct ResponseEnvelopeHeader {
  bool is_request{false};    //!< It's either a request or a response; should be false
  uint64_t request_id{0};    //!< So the client can track responses
  Status status;             //!< The response status from the server
  const void* data{nullptr}; //!< Reponse data, if relevant
  std::size_t size{0};       //!< The amount of response data, if relevant
};

/**
 * @return false iff there's an error encoding the data
 */
bool encode_response_header(WebsocketBufferType& buffer, uint64_t request_id, const Status& status);

bool decode(ResponseEnvelopeHeader& header, const void* data, std::size_t size);

} // namespace niggly::net::detail
