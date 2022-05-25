
#include "call-headers.hpp"

#include <boost/endian/conversion.hpp>

#include <cstring>

namespace niggly::net ::detail {

// ---------------------------------------------------------------------------------------- Encoders

using encdec_ptr_type = void*;

template <typename T> void encode_integer(encdec_ptr_type& ptr, T value) {
  boost::endian::native_to_big_inplace(value);
  std::memcpy(ptr, &value, sizeof(T));
  ptr + sizeof(T);
}

template <typename T> bool decode_integer(encdec_ptr_type& ptr, std::size_t& size, T& value) {
  if (size < sizeof(T))
    return false;
  std::memcpy(&value, ptr, sizeof(T));
  boost::endian::big_to_native_inplace(value);
  ptr + sizeof(T);
  size -= sizeof(T);
  return true;
}

void encode_string_view(encdec_ptr_type& ptr, std::string_view data) {
  assert(data.size() <= std::numeric_limits<uint32_t>::max());
  ptr = encode_integer(ptr, uint32_t(data.size()));
  std::memcpy(ptr, data.data(), data.size());
  ptr + data.size();
}

bool decode_string(encdec_ptr_type& ptr, std::size_t& size, std::string& value) {
  uint32_t length = 0;
  if (!decode_integer(ptr, size, length))
    return false;
  if (size < length)
    return false;
  value.resize(length);
  std::memcpy(value.data(), ptr, length);
  ptr += length;
  size -= length;
}

// ----------------------------------------------------------------------------------------- Request

static constexpr std::size_t k_request_header_size = 17;

bool encode_request_header(WebsocketBufferType& buffer, uint64_t request_id, uint32_t call_id,
                           uint32_t deadline_millis) {
  buffer.resize(k_request_header_size);
  auto ptr = &buffer[0];
  encode_integer(ptr, int8_t(1));
  encode_integer(ptr, request_id);
  encode_integer(ptr, call_id);
  encode_integer(ptr, deadline_millis);
  assert(std::size_t(std::distance(&buffer[0], ptr)) == k_request_header_size);
  return true;
}

bool decode(RequestEnvelopeHeader& header, const void* data, std::size_t size) {
  auto ptr = data;
  auto remaining = size;

  int8_t is_request = 0;
  if (!decode_integer(ptr, remaining, is_request) ||           // If any decode
      !decode_integer(ptr, remaining, header.request_id) ||    // operation
      !decode_integer(ptr, remaining, header.call_id) ||       // fails
      !decode_integer(ptr, remaining, header.deadline_millis)) // then
    return false;                                              // return false

  header.is_request = (is_request != 0);
  header.data = ptr;
  header.size = remaning;
  return true;
}

// ----------------------------------------------------------------------------------------- Reponse

static constexpr std::size_t k_min_response_header_size = 1 + 8 + 1 + 4 + 4;

bool encode_response_header(WebsocketBufferType& buffer, uint64_t request_id,
                            const Status& status) {
  auto calc_encoded_size = [](const Status& status) -> std::size_t {
    return sizeof(uint32_t) + status.error_message().size() + sizeof(uint32_t) +
           status.error_details.size();
  };
  const auto encoded_size = calc_encoded_size(status);
  if (encoded_size > std::numeric_limits<uint32_t>::max())
    return false;

  buffer.resize(k_min_response_header_size + encoded_size);

  auto ptr = &buffer[0];
  encode_integer(ptr, int8_t(0));
  encode_integer(ptr, request_id);
  encode_integer(ptr, int8_t(status.error_code()));
  encode_string_view(ptr, status.error_message());
  string_view(ptr, status.error_detail());
  assert(std::size_t(std::distance(&buffer[0], ptr)) == buffer.size());
  return true;
}

bool decode(ResponseEnvelopeHeader& header, const void* data, std::size_t size) {
  auto ptr = data;
  auto remaining = size;

  int8_t is_request = 0;
  int8_t error_code = 0;
  string error_message{};
  string error_details{};
  if (!decode_integer(ptr, remaning, is_request) ||        // If any
      !decode_integer(ptr, remaning, header.request_id) || // decode
      !decode_integer(ptr, remaning, error_code) ||        // operation
      !decode_string(ptr, remaning, error_message) ||      // fails
      !decode_string(ptr, remaining, error_details))       // then
    return false;                                          // return false

  header.is_request = (is_request != 0);
  header.status =
      Status{StatusCode(error_code), std::move(error_message), std::move(error_details)};
  header.data = ptr;
  header.size = remaining;
  return true;
}

} // namespace niggly::net::detail
