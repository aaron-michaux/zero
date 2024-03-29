
#include "stdinc.hpp"

#include "call-headers.hpp"

#include "niggly/net/buffer.hpp"

#include <boost/endian/conversion.hpp>

#include <limits>

#include <cstring>
#include <cassert>

namespace niggly::net ::detail {

// ---------------------------------------------------------------------------------------- Encoders

using enc_ptr_type = std::byte*;
using dec_ptr_type = const std::byte*;

template <typename T> void encode_integer(enc_ptr_type& ptr, T value) {
  boost::endian::native_to_big_inplace(value);
  std::memcpy(ptr, &value, sizeof(T));
  ptr += sizeof(T);
}

template <typename T> bool decode_integer(dec_ptr_type& ptr, std::size_t& size, T& value) {
  if (size < sizeof(T))
    return false;
  std::memcpy(&value, ptr, sizeof(T));
  boost::endian::big_to_native_inplace(value);
  ptr += sizeof(T);
  size -= sizeof(T);
  return true;
}

void encode_string_view(enc_ptr_type& ptr, std::string_view data) {
  assert(data.size() <= std::numeric_limits<uint32_t>::max());
  encode_integer(ptr, uint32_t(data.size()));
  std::memcpy(ptr, data.data(), data.size());
  ptr += data.size();
}

bool decode_string(dec_ptr_type& ptr, std::size_t& size, std::string& value) {
  uint32_t length = 0;
  if (!decode_integer(ptr, size, length))
    return false;
  if (size < length)
    return false;
  value.resize(length);
  std::memcpy(value.data(), ptr, length);
  ptr += length;
  size -= length;
  return true;
}

// ----------------------------------------------------------------------------------------- Request

static constexpr std::size_t k_request_header_size = 17;

bool encode_request_header(BufferType& buffer, uint64_t request_id, uint32_t call_id,
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

bool decode(RequestEnvelopeHeader& header, std::span<const std::byte> payload) {
  auto ptr = payload.data();
  auto remaining = payload.size();

  int8_t is_request = 0;
  if (!decode_integer(ptr, remaining, is_request) ||           // If any decode
      !decode_integer(ptr, remaining, header.request_id) ||    // operation
      !decode_integer(ptr, remaining, header.call_id) ||       // fails
      !decode_integer(ptr, remaining, header.deadline_millis)) // then
    return false;                                              // return false

  header.is_request = (is_request != 0);
  header.payload = std::span<const std::byte>{ptr, ptr + remaining};
  return true;
}

// ----------------------------------------------------------------------------------------- Reponse

static constexpr std::size_t k_min_response_header_size = 1 + 8 + 1 + 4 + 4;

bool encode_response_header(BufferType& buffer, uint64_t request_id, const Status& status) {
  auto calc_strings_size = [](const Status& status) -> std::size_t {
    return status.error_message().size() + status.error_details().size();
  };
  const auto strings_size = calc_strings_size(status);
  if (strings_size > std::numeric_limits<uint32_t>::max())
    return false;

  buffer.resize(k_min_response_header_size + strings_size);

  auto ptr = &buffer[0];
  encode_integer(ptr, int8_t(0));
  encode_integer(ptr, request_id);
  encode_integer(ptr, int8_t(status.error_code()));
  encode_string_view(ptr, status.error_message());
  encode_string_view(ptr, status.error_details());

  assert(std::size_t(std::distance(&buffer[0], ptr)) == buffer.size());
  return true;
}

bool decode(ResponseEnvelopeHeader& header, std::span<const std::byte> payload) {
  auto ptr = payload.data();
  auto remaining = payload.size();

  int8_t is_request = 0;
  int8_t error_code = 0;
  string error_message{};
  string error_details{};
  if (!decode_integer(ptr, remaining, is_request) ||        // If any
      !decode_integer(ptr, remaining, header.request_id) || // decode
      !decode_integer(ptr, remaining, error_code) ||        // operation
      !decode_string(ptr, remaining, error_message) ||      // fails
      !decode_string(ptr, remaining, error_details))        // then
    return false;                                           // return false

  header.is_request = (is_request != 0);

  if (error_code < int(StatusCode::OK) || error_code > int(StatusCode::DO_NOT_USE))
    return false;

  header.status =
      Status{StatusCode(error_code), std::move(error_message), std::move(error_details)};
  header.payload = std::span<const std::byte>{ptr, ptr + remaining};
  return true;
}

} // namespace niggly::net::detail
