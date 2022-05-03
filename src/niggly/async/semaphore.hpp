
#pragma once

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>

namespace niggly::async
{

/**
 * @ingroup async
 * @brief Efficient counting semaphore.
 * Uses atomics to maintain the count. When the count decrements below
 * zero, then uses a mutex and considtion-variable, for efficient
 * (potentially) long term locking.
 */
class CountingSemaphore
{
 private:
   class ConditionVariableSemaphore
   {
    private:
      int64_t count_;
      std::mutex padlock_;
      std::condition_variable cv_;

    public:
      ConditionVariableSemaphore(int64_t count)
          : count_(count)
      {}

      void post()
      {
         {
            std::unique_lock lock{padlock_};
            ++count_;
         }
         cv_.notify_one();
      }

      void wait()
      {
         std::unique_lock lock{padlock_};
         cv_.wait(lock, [&]() { return count_ != 0; });
         --count_;
      }

      void notify_all() { cv_.notify_all(); }
   };

   std::atomic<int64_t> count_;
   ConditionVariableSemaphore semaphore_;

 public:
   /**
    * @brief Construct a new counting semaphore with the specified initial
    *        count.
    * @param count Pass `0` for a binary semaphore. Otherwise must be positive.
    */
   CountingSemaphore(int64_t count)
       : count_(count)
       , semaphore_(0)
   {
      assert(count >= 0);
   }

   /**
    * @brief Wake all waiting threads without adjusting counts
    */
   void notify_all() { semaphore_.notify_all(); }

   /**
    * @brief Adds `1` to count, the equivalent of an `unlock`.
    */
   void post()
   {
      const auto old_count = count_.fetch_add(1, std::memory_order_release);
      if(old_count < 0) semaphore_.post();
   }

   /**
    * @brief Decrements `1` from count, the equivalent of a `lock`. Blocks
    *        the thread if the count is decremented below zero.
    */
   void wait()
   {
      const auto old_count = count_.fetch_sub(1, std::memory_order_acquire);
      if(old_count <= 0) semaphore_.wait();
   }
};

} // namespace niggly::async
