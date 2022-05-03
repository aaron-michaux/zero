
#pragma once

#include "thread-pool.hpp"
#include "timer-manager.hpp"

#ifdef USE_ASIO
#include <boost/asio.hpp>
#endif

namespace niggly::async
{

#ifdef USE_ASIO
namespace asio
{
   /**
    * @ingroup async
    * @brief An adaptor for executing on an asio io_context, with post and post_later
    */
   class ExecutionBroker
   {
    private:
      boost::asio::io_context& io_context_;

    public:
      /**
       * @brief Contruct with a backing io_context
       */
      ExecutionBroker(boost::asio::io_context& io_context)
          : io_context_{io_context}
      {}

      /**
       * @brief So that an ExecutionBroker can also be an Executor
       */
      template<typename F> void execute(F&& work) const
      {
         // This is a customization point, where we submit the work to the thread pool
         boost::asio::execution::execute(io_context_.get_executor(), std::forward<F>(work));
      }

      /**
       * @brief Post the work to the underlying io_context
       */
      template<typename F> void post(F&& work) const
      {
         // This is a customization point, where we submit the work to the thread pool
         boost::asio::execution::execute(io_context_.get_executor(), std::forward<F>(work));
      }

      /**
       * @brief Post the work for later execution
       */
      template<typename F, typename Rep, typename Period>
      Future<std::invoke_result_t<F>> post_later(const std::chrono::duration<Rep, Period>& duration,
                                                 F&& work) const
      {
         return post_later(std::allocator<int>{}, duration, std::forward<F>(work));
      }

      /**
       * @brief Post the work for later execution, using `alloc` to allocate
       */
      template<typename Allocator, typename F, typename Rep, typename Period>
      Future<std::invoke_result_t<F>> post_later(const Allocator& alloc,
                                                 const std::chrono::duration<Rep, Period>& duration,
                                                 F&& f) const
      {
         using R = std::invoke_result_t<F>;
         PackagedTask<R> work(std::allocator_arg_t{}, alloc, [f = std::move(f)]() { return f(); });
         Future<R> future = work.get_future();
         auto timer = std::allocate_shared<boost::asio::steady_timer>(alloc, io_context_, duration);
         auto thunk = [timer, work = std::move(work)](boost::system::error_code ec) mutable {
            if(!ec) {
               work();
            } else {
               work.cancel();
            }
         };
         timer->async_wait(std::move(thunk));
         return future;
      }
   };
} // namespace asio
#endif

/**
 * @ingroup async
 * @brief An adaptor for executing on a ThreadPool and TimerManager, with post and post_later
 */
template<typename Executor> class ExecutionBroker
{
 private:
   TimerManager<Executor>& timer_manager_;

 public:
   ExecutionBroker(TimerManager<Executor>& timer_manager)
       : timer_manager_{timer_manager}
   {}

   /**
    * @brief So that an ExecutionBroker can also be an Executor
    */
   template<typename F> void execute(F&& work) const
   {
      timer_manager_.get_executor().execute(std::forward<F>(work));
   }

   /**
    * @brief Post the work to the underlying io_context
    */
   template<typename F> void post(F&& work) const
   {
      timer_manager_.get_executor().execute(std::forward<F>(work));
   }

   /**
    * @brief Post the work for later execution
    */
   template<typename F, typename Rep, typename Period>
   Future<std::invoke_result_t<F>> post_later(const std::chrono::duration<Rep, Period>& duration,
                                              F&& work) const
   {
      return timer_manager_.post_later(duration, std::forward<F>(work));
   }

   /**
    * @brief Post the work for later execution, using `alloc` to allocate the future's shared state
    */
   template<typename Allocator, typename F, typename Rep, typename Period>
   Future<std::invoke_result_t<F>> post_later(const Allocator& alloc,
                                              const std::chrono::duration<Rep, Period>& duration,
                                              F&& work) const
   {
      return timer_manager_.post_later(alloc, duration, std::forward<F>(work));
   }
};

} // namespace niggly::async
