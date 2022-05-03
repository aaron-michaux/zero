
#pragma once

#include <atomic>
#include <thread>

#include <ctime>

namespace niggly::async
{
/**
 * @ingroup async
 * @brief Fair "first in first served" mutex substitude. Much bigger memory
 *        footprint than `SpinLock`.
 */
class TicketLock
{
 private:
   // Align 'in' and 'out' so that they appear on separate cache-lines.
   // This prevents concurrent accesses from interfering with each other.
   // std::hardware_destructive_interference_size

   /**
    * Prevents _false sharing_ for cache-lines, thus making access to `in_`,
    * and `_ot` much more efficient. `128` is a pessimistic value: twice
    * the cacheline size on modern x86_64 machines.
    */
   static constexpr std::size_t hardware_destructive_interference_size = 128;

   alignas(hardware_destructive_interference_size) std::atomic<int> in_{0};
   alignas(hardware_destructive_interference_size) std::atomic<int> ot_{0};

   static inline void nano_sleep_(unsigned nanos)
   {
      std::timespec ts;
      ts.tv_sec  = 0;
      ts.tv_nsec = nanos;
      nanosleep(&ts, nullptr);
   }

 public:
   /**
    * @brief Acquire the lock on a first-come first-served basis.
    */
   void lock()
   {
      std::atomic_thread_fence(std::memory_order_acquire);
      const auto ticket_no = in_.fetch_add(1, std::memory_order_relaxed);
      for(uint64_t attempts = 0; true; ++attempts) {
         const auto current_ticket = ot_.load(std::memory_order_acquire);
         if(ticket_no == current_ticket) break; // we have a lock

         if(attempts < 4)
            ; // try again
         else if(attempts < 64)
            std::this_thread::yield(); // back off
         else
            nano_sleep_(2000); // back off more (2 microseconds)
      }
   }

   /**
    * @brief Release the lock, making it available to the next "ticket" holder.
    */
   void unlock()
   {
      ot_.fetch_add(1, std::memory_order_relaxed);
      std::atomic_thread_fence(std::memory_order_release);
   }
};
} // namespace niggly::async
