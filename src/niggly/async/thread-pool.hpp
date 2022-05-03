
#pragma once

#include "executor-lock-guard.hpp"

#include <chrono>
#include <functional>

namespace niggly::async
{

class ThreadPool;
class ThreadPoolExecutor;

/**
 * @ingroup async
 * @brief Thread pool executor, with fixed capacity backing storage.
 *        Non-blocking, unless the backing storage is full. In which case,
 *        the calling thread can either block, or dequeue a task and run it
 *        before enqueuing a new task.
 */
class ThreadPool
{
 public:
   /**
    * @brief Policy for schedularing a thunk for execution. The `ThreadPool`
    *        is lock-free when backing storage exists to post a task.
    *        `Policy` is used to control behavior when backing storage is
    *        full. Most thread pools use the `BLOCK_WHEN_FULL` behavior, which
    *        causes the calling thread to block until storage becomes available.
    *        The `NEVER_BLOCK` policy will free storage by dequeing and
    *        executing a task.
    */
   enum class Policy : int {
      BLOCK_WHEN_FULL = 0, //!< Calling thread blocks if task queue is full.
      DISPATCH_WHEN_FULL,  //!< Dispatches task if it's a thread-pool thread otherwise NEVERBLOCK
      NEVER_BLOCK          //!< If task queue full, then increase threadpool size
   };

   using thunk_type = std::function<void()>;

 private:
   struct Pimpl;
   std::unique_ptr<Pimpl> pimpl_;

 public:
   ThreadPool(unsigned thread_count   = 0,
              unsigned n_queues       = 0,
              unsigned queue_capacity = 0); // bad alloc
   ThreadPool(const ThreadPool&) = delete;
   ThreadPool(ThreadPool&&)      = default;
   ~ThreadPool();
   ThreadPool& operator=(const ThreadPool&) = delete;
   ThreadPool& operator=(ThreadPool&&)      = default;

   void dispose();

   void post(thunk_type f);     // terminate on bad alloc
   void dispatch(thunk_type f); // terminate on bad alloc
   void defer(thunk_type f);    // terminate on bad alloc

   bool try_run_one(); // attempt to dequeue and run a task

   /**
    * Returns an executor for executing on the thread pool
    */
   ThreadPoolExecutor get_executor();

   /**
    * @brief A non-blocking way to lock `mutex`. While the mutex lock is
    *        not obtained, the thread-pool continues to execute jobs --
    *        via `try_run_one()`.
    * Caution: latency will be high if a lock running job is executed.
    * Caution: obtaining multiple locks could easily cause a deadlock
    */
   template<typename mutex_type> auto lock(mutex_type& mutex)
   {
      return ExecutorLockGuard<ThreadPool, mutex_type>(*this, mutex);
   }
};

/**
 * @ingroup async
 * @brief A wrapper around whatever form of execution agent you have.
 */
class ThreadPoolExecutor
{
 private:
   ThreadPool& pool_;

 public:
   ThreadPoolExecutor(ThreadPool& pool)
       : pool_{pool}
   {}
   template<class F> void execute(F&& f) const noexcept { pool_.post(std::forward<F>(f)); }

   bool operator==(const ThreadPoolExecutor& o) const noexcept { return &pool_ == &o.pool_; }
};

} // namespace niggly::async
