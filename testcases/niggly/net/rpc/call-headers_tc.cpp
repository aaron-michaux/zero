
#include "stdinc.hpp"

#include "niggly/net/rpc/call-headers.hpp"

#include <catch2/catch.hpp>

namespace niggly::net::detail::test {

// ------------------------------------------------------------------- TEST_CASE
//
CATCH_TEST_CASE("rpc-headers", "[rpc-headers]") {
  //

  CATCH_SECTION("rpc-request-headers") {

    auto test_request_header = [](BufferType& buffer, uint64_t request_id, uint32_t call_id,
                                  uint32_t deadline_millis) {
      RequestEnvelopeHeader header;
      CATCH_REQUIRE(encode_request_header(buffer, request_id, call_id, deadline_millis));
      CATCH_REQUIRE(decode(header, {buffer.data(), buffer.data() + buffer.size()}));
      CATCH_REQUIRE(header.is_request == true);
      CATCH_REQUIRE(header.request_id == request_id);
      CATCH_REQUIRE(header.call_id == call_id);
      CATCH_REQUIRE(header.deadline_millis == deadline_millis);
      CATCH_REQUIRE(header.payload.size() == 0);
    };

    BufferType buffer;
    for (auto request_id = 0; request_id < 2000; request_id += 1000)
      for (auto call_id = 0; call_id < 2000; call_id += 1000)
        for (auto deadline_millis = 0; deadline_millis < 2000; deadline_millis += 1000)
          test_request_header(buffer, request_id, call_id, deadline_millis);
  }

  CATCH_SECTION("rpc-response-headers") {

    auto test_response_header = [](BufferType& buffer, uint64_t request_id, Status status) {
      ResponseEnvelopeHeader header;
      CATCH_REQUIRE(encode_response_header(buffer, request_id, status));
      CATCH_REQUIRE(decode(header, {buffer.data(), buffer.data() + buffer.size()}));
      CATCH_REQUIRE(header.is_request == false);
      CATCH_REQUIRE(header.request_id == request_id);
      CATCH_REQUIRE(header.status == status);
      CATCH_REQUIRE(header.payload.size() == 0);
    };

    BufferType buffer;
    for (auto request_id = 0; request_id < 2000; request_id += 1000) {
      test_response_header(buffer, request_id, Status{});
      test_response_header(buffer, request_id, Status{StatusCode::OK});
      test_response_header(buffer, request_id, Status{StatusCode::ABORTED});
      test_response_header(buffer, request_id, Status{StatusCode::DO_NOT_USE});
      test_response_header(buffer, request_id, Status{StatusCode::DO_NOT_USE, "message"});
      test_response_header(buffer, request_id,
                           Status{StatusCode::DO_NOT_USE, "message", "details"});
    }
  }
}

} // namespace niggly::net::detail::test
