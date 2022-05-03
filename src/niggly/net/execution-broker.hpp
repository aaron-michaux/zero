
#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <memory>

namespace niggly::net
{

/**
 * Type erase the underlying boost::asio::io_context
 */
class ExecutionBroker
{
 private:
   boost::asio::io_context& io_context_;

 public:
   ExecutionBroker(boost::asio::io_context& io_context)
       : io_context_{io_context}
   {}

   template<typename F> void post(F&& work) const
   {
      // This is a customization point, where we submit the work to the thread pool
      boost::asio::execution::execute(io_context_.get_executor(), std::forward<F>(work));
   }

   template<typename F, typename Rep, typename Period>
   void post_later(const std::chrono::duration<Rep, Period>& duration, F&& work) const
   {
      auto timer = std::make_shared<boost::asio::steady_timer>(io_context_, duration);
      auto thunk = [timer, work = std::forward<F>(work)](boost::system::error_code ec) mutable {
         if(!ec) {
            work();
         } else {
            work.cancel();
         }
      };

      timer->async_wait(std::move(thunk));
   }
};

} // namespace niggly::net
