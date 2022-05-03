
#pragma once

#include "execution-broker.hpp"
#include "extended-futures.hpp"

namespace niggly::async
{

///@{ @name async_thunk
/**
 * @ingroup async
 * @brief Execuate the callable with the passed `executor`, returning a Future to the result.
 * @return a Future to the result.
 *
 * @include extended-futures_ex.cpp
 */
template<typename Executor, typename F>
Future<std::invoke_result_t<F>> async(const Executor& executor, F&& f)
{
   return async(executor, std::allocator<int>{}, std::forward<F>(f));
}

/**
 * @ingroup async
 * @brief Execuate the callable with the passed `executor`, using `alloc` to allocate the shared
 *        state.
 * @return a Future to the result.
 */
template<typename Executor, typename Allocator, typename F>
Future<std::invoke_result_t<F>> async(const Executor& executor, const Allocator& alloc, F&& f)
{
   // package up the work
   using R = std::invoke_result_t<F>;
   PackagedTask<R> work(std::allocator_arg_t{}, alloc, [f = std::forward<F>(f)]() { return f(); });
   Future<R> result = work.get_future();
   executor.execute(std::move(work));
   return result;
}

/**
 * @ingroup async
 * @brief Execuate the callable with the passed ExecutionBroker, an adaptor that supports the
 *        `post_later` method.
 * @return a Future to the result.
 */
template<typename ExecutionBroker, typename F, typename Rep, typename Period>
Future<std::invoke_result_t<F>> async_later(const ExecutionBroker& executor,
                                            const std::chrono::duration<Rep, Period>& duration,
                                            F&& f)
{
   return executor.post_later(duration, std::forward<F>(f));
}

/**
 * @ingroup async
 * @brief Execuate the callable with the passed ExecutionBroker, an adaptor that supports the
 *        `post_later` method; uses `alloc` to allocate the shared state.
 * @return a Future to the result.
 */
template<typename ExecutionBroker, typename Allocator, typename F, typename Rep, typename Period>
Future<std::invoke_result_t<F>> async_later(const ExecutionBroker& executor,
                                            const Allocator& alloc,
                                            const std::chrono::duration<Rep, Period>& duration,
                                            F&& f)
{
   return executor.post_later(alloc, duration, std::forward<F>(f));
}

///@}

} // namespace niggly::async
