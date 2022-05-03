
#pragma once

#include "websocket-session.hpp"

#include "niggly/utils.hpp"

namespace boost::asio
{
class io_context;
}

namespace niggly::net
{

// --------------------------------------------------------------------------------- WebsocketServer

class WebsocketServer
{
 private:
   struct Pimpl;
   std::unique_ptr<Pimpl> pimpl_;

 public:
   struct Config
   {
      string address                = "0.0.0.0"; //! List address
      uint16_t port                 = 0;         //! List port
      string dh_file                = {};        //! For key-exchange
      string certificate_chain_file = {};        //! Server certificate
      string private_key_file       = {};        //! Primate key

      /**
       * @brief A callback for associating logic with an underlying websocket session.
       * @note Must be set.
       */
      std::function<std::shared_ptr<WebsocketSession>()> session_factory;

      /**
       * @brief A callback that is called when a new connection starts.
       * This method is called with the result of `session_factor`.
       * @note Setting this is optional
       */
      std::function<void(std::shared_ptr<WebsocketSession>)> on_new_session;
   };

   /**
    * Exceptions
    * + std::bad_alloc
    * + std::runtime_error When listening on the configured port
    */
   WebsocketServer(boost::asio::io_context& io_context, const Config& config);
   WebsocketServer(const WebsocketServer&) = delete;
   WebsocketServer(WebsocketServer&&)      = default;
   ~WebsocketServer();
   WebsocketServer& operator=(const WebsocketServer&) = delete;
   WebsocketServer& operator=(WebsocketServer&&)      = default;

   /**
    * @brief Start listening on the configured port
    * @param on_new_session Called when a new connection starts.
    * @param on_message Called when a new message is received - see notes.
    */
   std::error_code run();
};

} // namespace niggly::net
