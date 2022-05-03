
#pragma once

/*
In a nutshell
 * Clients and Servers, TLS (done)
 * async event loop -- template provide the executor (done)
 * The client connect to the server, establishing 2-way communication (done)
 * Send messages, message: { size, payload } (done)
 * Get documents (HTTP like)
 * Rrpc: { message-id-in, message-out-id, function name } => result function(args)
 *
 *
 *
 */

#include "niggly/net/execution-context.hpp"
#include "niggly/net/websocket-server.hpp"

/*
  template<uint32_t request_id,
           typename Executor,
           typename Allocator, // For the
           typename RequestSerializer,
           typename ResponseSerializer,
           typename Function>

When a WebsocketSession starts, it is not associated with client-state
It then gets associated with a client-state (this allows reconnections, multiple connections)
We then have "Client-State" object which is the service point for maintaining world-state

* The rpc system requires a standard format:
  + request { rpc-id, request-id, payload }
  + response { rpc-id, request-id, status, payload? }
* The rpc-id maps to the funciton.
* We only serialize/deserialize the payload for the function.
* The request-id is used to track the async request...
* And `status` is used to return errors. So we may not have a payload in the response if there's a
timeout.

Want to support in process "rpc", where the serialization/send step is skipped
 */
