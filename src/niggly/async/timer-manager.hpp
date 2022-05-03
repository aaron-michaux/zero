
#pragma once

#include "extended-futures.hpp"

#include "niggly/utils/base-include.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <numeric>
#include <queue>
#include <vector>

namespace niggly::async
{
namespace detail
{
   static constexpr auto k_sentinal_time = std::chrono::steady_clock::time_point::max();

   class WorkQueue
   {
    public:
      using thunk_type = niggly::function<void()>;
      using time_point = std::chrono::steady_clock::time_point;

    private:
      struct Work
      {
         Work()                           = default;
         Work(const Work&)                = delete;
         Work(Work&&) noexcept            = default;
         ~Work()                          = default;
         Work& operator=(const Work&)     = delete;
         Work& operator=(Work&&) noexcept = default;
         Work(thunk_type&& thunk_, time_point when_)
             : thunk{std::move(thunk_)}
             , when{when_}
         {}
         thunk_type thunk;
         time_point when;
         bool operator<(const Work& o) const noexcept { return when < o.when; }
      };
      using queue_type = std::vector<Work>;

      queue_type queue_;
      mutable std::mutex padlock_;

      const Work& top_locked_() const
      {
         assert(queue_.size() > 0);
         return queue_.front();
      }

      void pop_locked_(thunk_type& x)
      {
         assert(queue_.size() > 0);
         std::pop_heap(begin(queue_), end(queue_));
         x = std::move(queue_.back().thunk);
         queue_.pop_back();
      }

      bool try_pop_locked_(const time_point now, thunk_type& x)
      {
         if(queue_.size() > 0 && top_locked_().when >= now) {
            pop_locked_(x);
            return true;
         }
         return false;
      }

      void push_locked_(time_point when, thunk_type&& f)
      {
         assert(when != k_sentinal_time);
         try {
            queue_.emplace_back(std::move(f), when);
            std::push_heap(begin(queue_), end(queue_));
         } catch(...) {
            FATAL("exception copying thunk. Please make all thunks noexcept "
                  "copyable and noexcept movable");
         }
      }

    public:
      // Not threadsafe
      void reserve(uint32_t capacity) { queue_.reserve(capacity); }

      template<typename Executor> time_point pop_until(const time_point now, Executor& executor)
      {
         std::unique_lock lock{padlock_};
         thunk_type thunk;
         while(try_pop_locked_(now, thunk)) executor.execute(std::move(thunk));
         return queue_.size() == 0 ? k_sentinal_time : top_locked_().when;
      }

      bool push(time_point when, thunk_type&& f)
      {
         std::unique_lock lock{padlock_};
         push_locked_(when, std::move(f));
         return true;
      }

      bool try_push(time_point when, thunk_type&& f)
      {
         std::unique_lock lock{padlock_, std::try_to_lock};
         if(!lock) return false;
         push_locked_(when, std::move(f));
         return true;
      }
   };
} // namespace detail

/**
 * @ingroup async
 * @brief A thread that manages delayed execution of thunks.
 *
 * Call `post_later` or `async_later` to execute a thunk at some time in the
 * future. The TimerManager has a backing thread that managers the deadlines
 * for executing the thunks. When the execution time comes, the thunk is posted
 * to an execution context using the passed Executor.
 */
template<typename Executor> class TimerManager
{
 private:
   struct Pimpl
   {
    private:
      using thunk_type = std::function<void()>;
      using time_point = std::chrono::steady_clock::time_point;
      using WorkQueue  = detail::WorkQueue;

      std::unique_ptr<WorkQueue[]> queues_ = nullptr; //!< work queu array
      unsigned n_queues_                   = 0;       //!< number of work queues
      std::atomic<unsigned> push_index_    = 0;       //!< next queue to push to
      std::atomic<bool> is_done_           = false;   //!< queue is closing
      std::atomic<time_point> next_when_   = detail::k_sentinal_time;

      std::unique_ptr<std::thread> thread_; //!< For managing the timers
      std::condition_variable cv_{};        //!< For sleeping the working thread
      std::mutex padlock_{};                //!< For managing the condition variable

      Executor executor_;

      time_point pop_until_(Executor& executor, time_point when)
      {
         time_point next_when = detail::k_sentinal_time;
         for(auto i = 0u; i != n_queues_; ++i) {
            auto& queue         = queues_[i];
            const auto new_when = queue.pop_until(when, executor);
            if(new_when < next_when) next_when = new_when;
         }
         return next_when;
      }

      bool update_next_when_(time_point when)
      {
         bool did_update    = false;
         time_point current = next_when_.load(std::memory_order_acquire);
         if(when < current) {
            std::unique_lock lock{padlock_};
            if(when < next_when_.load(std::memory_order_acquire)) {
               next_when_.store(when, std::memory_order_release);
               did_update = true;
               cv_.notify_all();
            }
         }
         return did_update;
      }

      void wait_()
      {
         auto stop_waiting = [this]() {
            const auto delta
                = next_when_.load(std::memory_order_acquire) - std::chrono::steady_clock::now();
            return delta.count() <= 0 || is_done_.load(std::memory_order_acquire);
         };
         if(stop_waiting()) return;
         {
            std::unique_lock lock{padlock_};
            cv_.wait_until(lock, next_when_.load(std::memory_order_acquire), stop_waiting);
         }
      }

      void run_management_thread_()
      {
         auto is_done = [this]() { return is_done_.load(std::memory_order_acquire); };
         while(!is_done()) {
            next_when_.store(detail::k_sentinal_time, std::memory_order_release);
            const auto now       = std::chrono::steady_clock::now();
            const auto next_when = pop_until_(executor_, now);
            update_next_when_(next_when);
            wait_();
         }
      }

    public:
      Pimpl(Executor executor, unsigned number_queues = 8, unsigned queue_capacity = 100)
          : queues_{new WorkQueue[number_queues]}
          , n_queues_{number_queues}
          , executor_{executor}
      {
         for(auto i = 0u; i < number_queues; ++i) queues_[i].reserve(queue_capacity);
         thread_ = std::make_unique<std::thread>([this]() { run_management_thread_(); });
      }
      ~Pimpl() { dispose(); }

      Executor get_executor() { return executor_; }

      void dispose()
      {
         bool do_dispose = false;
         {
            bool expected_done = false;
            std::unique_lock lock{padlock_};
            do_dispose
                = is_done_.compare_exchange_strong(expected_done, true, std::memory_order_acq_rel);
            if(do_dispose) cv_.notify_all();
         }
         if(do_dispose) thread_->join();
      }

      bool post(std::chrono::nanoseconds offset, thunk_type&& thunk)
      {
         if(is_done_.load(std::memory_order_acquire)) return false;
         const auto now  = std::chrono::steady_clock::now();
         const auto when = now + offset;
         for(bool did_push = false; !did_push;) {
            const auto offset = push_index_.fetch_add(1, std::memory_order_relaxed);
            for(auto i = 0u; i != n_queues_ && !did_push; ++i)
               if(queues_[(offset + i) % n_queues_].try_push(when, std::move(thunk)))
                  did_push = true;
         }
         update_next_when_(when);
         return true;
      }
   };

   std::unique_ptr<Pimpl> pimpl_;

 public:
   TimerManager(Executor executor, unsigned number_queues = 8, unsigned queue_capacity = 100)
       : pimpl_{std::make_unique<Pimpl>(executor, number_queues, queue_capacity)}
   {}
   TimerManager(const TimerManager&)            = delete;
   TimerManager(TimerManager&&)                 = default;
   ~TimerManager()                              = default;
   TimerManager& operator=(const TimerManager&) = delete;
   TimerManager& operator=(TimerManager&&)      = default;

   /**
    * @brief Safely dispose of the internal work queue and thread
    * In order to prevent the TimerManager from using the Executor, you must
    * dispose of the TimerManager before disposing of whatever resource the Executor uses.
    */
   void dispose() { pimpl_->dispose(); }

   Executor get_executor() { return pimpl_->get_executor(); }

   /**
    * @brief Push 1 thunk for later execution
    * @return True iff successful; only fails if the TimerManger is being destructed
    */
   bool post(std::chrono::nanoseconds offset, thunk_type&& thunk)
   {
      return pimpl_->post(offset, std::move(thunk));
   }

   /**
    * @brief Schedule later execution, returning a cancellable future
    */
   template<typename F, typename Rep, typename Period>
   Future<std::invoke_result_t<F>> post_later(const std::chrono::duration<Rep, Period>& duration,
                                              F f)
   {
      return post_later(std::allocator<int>{}, duration, std::move(f));
   }

   /**
    * @brief Schedule later execution, using `alloc` to allocate the frame, returning a
    * cancellable thunk
    */
   template<typename Allocator, typename F, typename Rep, typename Period>
   Future<std::invoke_result_t<F>>
   post_later(const Allocator& alloc, const std::chrono::duration<Rep, Period>& duration, F f)
   {
      using R = std::invoke_result_t<F>;
      PackagedTask<R> task(std::allocator_arg_t{}, alloc, [f = std::move(f)]() { return f(); });
      Future<R> future = task.get_future();
      pimpl_->post(duration, std::move([work = std::move(task)]() mutable { work(); }));
      return future;
   }
};

} // namespace niggly::async
