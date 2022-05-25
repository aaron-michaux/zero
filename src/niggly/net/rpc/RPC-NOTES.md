
# TODO
 * The server time-out timer
 * Writing

# Transports Types

These types are serialized in network byte order

 * Envelope:    { uint64_t rpc_id, uint32_t call_id, uint32_t ttl_millis, <payload> }
 * Response:    { uint64_t rpc_id, Status status, <payload> }
 * Status:      { uint8_t code, String, String }
 * String/Blob: { utf-8-like-length, <chars> }

# Types

Status: (models grpc::Status) //
 * StatusCode code
 * string error_message
 * string error_details

Payloads:
 * Binary stream... must provide a serializer/deserializer

RpcAgent:
 * Struct maps {call_id -> Handler}
 * Executor

Handler:
 * thunk<Response (RpcContext&, Arg&)>
 * Arg Deserializer
 * Response Serializer
 * Maps to <thunk<void()> (const void* data, std::size_t size)>,
   which is what's 

(De)Serializer: one of:
 * thunk<T (const void* data, size_t size)>
 * thunk<void (const T&, std::ostream<std::byte>&)> (???)

CallContext:
 * client's request-id.
 * deadline.
 * bool is_cancelled.
 * void try_cancel.
 * set_completion.
 * A timer that cancels the call.

# Logic

 1. The server/client each sit in a WebsocketSession.
 2. On 'on_receive', the call is deserialized, and matched to a Handler.
 3. A timer is started, to manage call timeout.
 4. The Handler deserializes the Arg, and moves the arg and thunk the Executor.
 5. The Handler then, sometime later, will respond.
    If the timer fires, then the call is cancelled, and we send an error response.
    If the response comes in later, then it is ignored.
 6. We serialize and send the response.
 
 
 

