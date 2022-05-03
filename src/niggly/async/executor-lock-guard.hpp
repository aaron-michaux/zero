
#pragma once

#include <cstdint>
#include <ctime>
#include <thread>

namespace niggly::async
{
/**
 * @ingroup threads
 * @brief Lock guard runs executor jobs while waiting for lock.
 *        The executor must have the `try_run_one()` method. This
 *        method is called if `try_lock()` fails, making the
 *        `ExecutorLockGuard` non-blocking, but potentially high
 *        latency.
 */
template<typename Executor, typename mutex_type> class ExecutorLockGuard
{
 private:
   Executor& executor_;
   mutex_type& mutex_;

   static inline void nano_sleep_(unsigned nanos)
   {
      std::timespec ts;
      ts.tv_sec  = 0;
      ts.tv_nsec = nanos;
      nanosleep(&ts, nullptr);
   }

 public:
   explicit ExecutorLockGuard(Executor& executor, mutex_type& mutex)
       : executor_(executor)
       , mutex_(mutex)
   {
      for(uint64_t attempts = 0; !mutex_.try_lock(); ++attempts) {
         if(attempts < 4) {
            ; // just try again
         } else {
            if(executor_.try_run_one()) {
               ; // just try again
            } else if(attempts < 64) {
               std::this_thread::yield(); // back off
            } else {
               nano_sleep_(2000); // back off more (2 microseconds)
            }
         }
      }
   }

   ~ExecutorLockGuard() { mutex_.unlock(); }

   ExecutorLockGuard(ExecutorLockGuard const&)            = delete;
   ExecutorLockGuard& operator=(ExecutorLockGuard const&) = delete;
};

} // namespace niggly::async
