
#include "thread-pool.hpp"

#include "semaphore.hpp"
#include "spinlock.hpp"

#include "niggly/core.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <random>
#include <thread>
#include <vector>

#ifndef FATAL
#define FATAL(message_fmt, ...)                                                 \
   {                                                                            \
      std::cerr << fmt::format("[\x1b[4m\x1b[97m{}:{}:{}\x1b[0m] " message_fmt, \
                               __FILE__,                                        \
                               __LINE__,                                        \
                               __PRETTY_FUNCTION__,                             \
                               ##__VA_ARGS__)                                   \
                << std::endl;                                                   \
      std::terminate();                                                         \
   }
#endif

namespace niggly::async
{
namespace detail
{
   // -------------------------------------------------------- NonBlocking Queue

   /**
    * @private
    * @brief A lockfree fixed size queue, with an optimistic try_push and
    *        try_pop interface
    *
    * Backing storage is a fixed sized array. Pop/push increments either
    * the `start_` or `finish_` iterators, wrapping at `end_store`. The
    * size is stored separately (in `size_`) to distinguish between an
    * empty and full queue.
    *
    * Push operations always fail (return `false`) if the queue is full.
    * Likewise, pop operations always fail (return `false`) when the queue
    * is empty.
    *
    * The try push/pop interface optimistically attempts to lock `padlock_`,
    * and fails (returns `false`) if a lock is not acquired.
    */
   class NonBlockingQueue final
   {
    private:
      using thunk_type = ThreadPool::thunk_type;
      using iterator   = thunk_type*;

      iterator begin_store_ = nullptr; //! Beginning of memory
      iterator start_       = nullptr; //! First item in queue
      iterator finish_      = nullptr; //! Place to write next item... never equal to `end_store_`
      iterator end_store_   = nullptr; //! Past end of memory
      uint32_t size_        = 0;       //! Number of items in queue

      std::mutex padlock_;

      /**
       * Preconditions:
       * + `padlock_` must be locked
       */
      bool is_empty_() const { return size_ == 0; }

      /**
       * Preconditions:
       * + `padlock_` must be locked
       */
      bool is_full_() const { return capacity_() == size_; }

      /**
       * Preconditions:
       * + `padlock_` must be locked
       */
      uint32_t capacity_() const { return uint32_t(end_store_ - begin_store_); }

      /**
       * Preconditions:
       * + `padlock_` must be locked
       */
      void push_(thunk_type&& f)
      {
         assert(!is_full_());
         assert(f);
         inplace_move_construct_(std::move(f), finish_++);
         if(finish_ == end_store_) finish_ = begin_store_;
         ++size_;
         assert(!is_empty_());
         assert(!is_full_() || start_ == finish_);
      }

      /**
       * Preconditions:
       * + `padlock_` must be locked
       */
      void pop_(thunk_type& x)
      {
         assert(!is_empty_());
         assert(*start_);
         try {
            x = std::move(*start_);
            std::destroy_at(start_++);
         } catch(...) {
            FATAL("exception copying thunk. Please make all thunks noexcept "
                  "copyable and noexcept movable");
         }
         if(start_ == end_store_) start_ = begin_store_;
         --size_;
         assert(!is_full_());
         assert(!is_empty_() || start_ == finish_);
      }

      void dispose_()
      {
         if(begin_store_ != nullptr) {
            while(size_ > 0) {
               std::destroy_at(start_++);
               if(start_ == end_store_) start_ = begin_store_;
               --size_;
            }
            std::free(begin_store_);
         }
      }

      void inplace_move_construct_(thunk_type&& thunk, thunk_type* write_pos)
      {
         try {
            new(write_pos) thunk_type(std::move(thunk));
         } catch(...) {
            FATAL("exception copying thunk. Please make all thunks noexcept "
                  "copyable and noexcept movable");
         }
      }

      void inplace_move_construct_(thunk_type* read_pos, thunk_type* write_pos)
      {
         inplace_move_construct_(std::move(*read_pos), write_pos);
      }

      void reserve_(const uint32_t new_capacity)
      {
         if(new_capacity <= (end_store_ - begin_store_)) return;

         auto new_begin  = static_cast<thunk_type*>(std::malloc(new_capacity * sizeof(thunk_type)));
         auto new_end    = new_begin + new_capacity;
         auto new_start  = new_begin;
         auto new_finish = new_begin + size_; // The next empty writable spot.
         assert(new_finish != new_end);       // capacity has increased

         { // copy across
            auto read_pos  = start_;
            auto write_pos = new_start;
            while(write_pos != new_finish) {
               inplace_move_construct_(read_pos++, write_pos++);
               if(write_pos == end_store_) write_pos = begin_store_;
            }
         }

         // Free resources
         std::free(begin_store_);

         begin_store_ = new_begin;
         end_store_   = new_end;
         start_       = new_start;
         finish_      = new_finish;

         assert(!is_full_() || start_ == finish_);
      }

    public:
      NonBlockingQueue(const uint32_t capacity = 0) { init(capacity); }

      ~NonBlockingQueue() { dispose_(); }

      /**
       * @brief Initializes (or reinitializes) the queue to a fixed capacity
       *
       * Exceptions:
       * + std::bad_alloc, failed to allocate memory for backing storage
       *
       * Preconditions:
       * + None
       */
      void init(const uint32_t capacity) // std::bad_alloc
      {
         dispose_();
         begin_store_ = end_store_ = start_ = finish_ = nullptr;
         size_                                        = 0;
         reserve_(capacity);
      }

      /**
       * @brief Attempts to `std::move` the start of the queue into `x`
       * Pop fails if the queue is empty, or `try_lock` fails to acquire a
       * lock
       *
       * THREAD_SAFE
       *
       * @return `true` if, and only if, the pop is successful
       *
       * Preconditions:
       * + `x` is not executable.
       *
       * Postconditions:
       * + `x` is an executable thunk if, and only if, `true` is returned.
       */
      bool try_pop(thunk_type& x)
      {
         std::unique_lock lock{padlock_, std::try_to_lock};
         if(!lock || is_empty_()) return false;
         pop_(x);
         return true;
      }

      /**
       * @brief Attempts to push `f` into the queue.
       * Push fails if the queue is full, or `try_lock` fails to acquire a
       * lock
       *
       * THREAD_SAFE
       *
       * @return `true` if, and only if, the push is successful
       *
       * Preconditions:
       * + `f` is an executable thunk
       *
       * Preconditions:
       * + `f` has been moved from if, and only if, `true` is returned
       */
      bool try_push(thunk_type&& f)
      {
         std::unique_lock lock{padlock_, std::try_to_lock};
         if(!lock || is_full_()) return false;
         push_(std::move(f));
         return true;
      }

      /**
       * @brief Attempts to push `f` into the queue.
       * If the queue is full, then the head of the queue is `std::moved`
       * into `out_f`.
       *
       * Only fails when `try_lock` fails to acquire a lock
       *
       * THREAD_SAFE
       *
       * @return `true` if, and only if, the push is successful
       *
       * Preconditions:
       * + `f` is an executable thunk
       * + `out_f` is not executable -- i.e., `if(out_f)` is `false`.
       *
       * Postconditions:
       * + `f` has been moved from if, and only if, `true` is returned
       * + `out_f` is executable if, and onf if, `true` is returned
       */
      bool try_push_or_exchange(thunk_type&& f, thunk_type& out_f)
      {
         assert(f);
         using std::swap;

         std::unique_lock lock{padlock_, std::try_to_lock};
         if(!lock) return false;

         if(is_full_()) pop_(out_f);
         push_(std::move(f));
         return true;
      }

      std::pair<uint32_t, bool> try_push_with_resize(thunk_type&& f)
      {
         assert(f);
         std::unique_lock lock{padlock_, std::try_to_lock};
         if(!lock) return {0, false};

         uint32_t delta_capacity = 0;
         if(is_full_()) {
            const auto new_capacity = uint32_t(double(size_) * 2.1);
            delta_capacity          = new_capacity - capacity_();
            assert(delta_capacity > 0);
            reserve_(new_capacity);
         }

         push_(std::move(f));
         return {delta_capacity, true};
      }
   };

   // ------------------------------------------------------- Notification Queue
   /**
    * @private
    * @brief A blocking queue, with notifications for pushes and pops
    */
   class NotificationQueue final
   {
    private:
      using lock_t     = std::unique_lock<std::mutex>;
      using thunk_type = ThreadPool::thunk_type;

      std::unique_ptr<NonBlockingQueue[]> queues_ = nullptr;
      unsigned n_queues_                          = 0;

      std::atomic<unsigned> push_index_ = 0; //!< next queue to push to
      std::atomic<unsigned> pop_index_  = 0; //!< next queue to pop from

      std::atomic<std::size_t> size_     = 0;
      std::atomic<std::size_t> capacity_ = 0;
      std::atomic<bool> is_done_         = false;

      std::mutex data_padlock_;
      std::condition_variable data_cv_;

      void dispose_() { signal_done(); }

    public:
      /**
       * Construct a notification queue whose backing storage consists
       * of `number_queues` queues, each with a fixed capacity of
       * `queue_capacity`. (Total capacity is `number_queues *
       * queue_capacity`.)
       *
       * Exceptions:
       * + `std::bad_alloc` if allocation fails.
       */
      NotificationQueue(unsigned number_queues, unsigned queue_capacity)
          : queues_{new NonBlockingQueue[number_queues]}
          , n_queues_{number_queues}
          , size_{0}
          , capacity_{queue_capacity * number_queues}
          , is_done_{false}
      {
         for(auto n = 0u; n < n_queues_; ++n) queues_[n].init(queue_capacity);
      }

      /**
       * @brief calls `signal_done()`, and then disposes the queue after
       *        all items are popped.
       */
      ~NotificationQueue() { dispose_(); }

      /**
       * @brief Signals that no more elements should be pushed.
       *
       * THREAD SAFE
       *
       * Once `signal_done()` has been called, then `pop()` will return
       * `false` once the queue is empty. It is safe to delete the queue at
       * this point.
       */
      void signal_done()
      {
         std::unique_lock lock{data_padlock_};
         is_done_.store(true, std::memory_order_release);
         data_cv_.notify_all();
      }

      bool done_is_signalled() const noexcept { return is_done_.load(std::memory_order_acquire); }

      std::size_t size() const noexcept { return size_.load(std::memory_order_acquire); }
      std::size_t capacity() const noexcept { return capacity_.load(std::memory_order_acquire); }

    private:
      void note_increased_capacity_(std::size_t delta)
      {
         capacity_.fetch_add(delta, std::memory_order_acq_rel);
      }

      void decrement_size_()
      {
         const auto old_size = size_.fetch_sub(1, std::memory_order_acq_rel);
         assert(old_size >= 1);
      }

      void increment_size_()
      {
         const auto old_size = size_.fetch_add(1, std::memory_order_acq_rel);
         if(old_size == 0) { data_cv_.notify_all(); }
      }

      bool try_pop_(thunk_type& thunk)
      {
         const auto offset = pop_index_.fetch_add(1, std::memory_order_relaxed);
         for(auto i = 0u; i != n_queues_; ++i) {
            auto& queue = queues_[(offset + i) % n_queues_];
            if(queue.try_pop(thunk)) {
               decrement_size_();
               return true;
            }
         }
         return false;
      }

    public:
      /**
       * @brief Push `thunk` onto the queue. If the queue is full, then
       *        pop the start of the queue before pushing, and execute
       *        the popped thunk.
       *
       * THREAD SAFE
       *
       * Non-blocking push provides an alternative mechanism to slow
       * down over-active producers of thunks. Normally, a task queue
       * would be required to block the producer, or allocate more
       * memory for storage -- until memory is exhausted. Backing storage is
       * fixed size for a non-blocking push, and, instead of blocking the
       * producer, we free up storage by executing one of the pending thunks.
       *
       * This means that some thunks will be executed outside of the
       * thread-pool.
       */
      bool non_blocking_push(thunk_type&& thunk)
      {
         assert(thunk);
         if(is_done_.load(std::memory_order_acquire)) return false;

         thunk_type out_thunk;

         for(bool did_push = false; !did_push;) {
            const auto offset = push_index_.fetch_add(1, std::memory_order_relaxed);

            for(auto i = 0u; i != n_queues_ && !did_push; ++i)
               if(queues_[(offset + i) % n_queues_].try_push_or_exchange(std::move(thunk),
                                                                         out_thunk))
                  did_push = true;
         }

         if(out_thunk) {
            // Queue size is unchanged
            try {
               thunk(); // And execute the thunk
            } catch(...) {
               FATAL("logic error: exception in thunk");
            }
         } else {
            increment_size_(); // Queue size has increased, may notify waiters
         }

         return true;
      }

      /**
       * @brief Push `thunk` onto the queue. Increases storage if unavailable.
       *
       * THREAD SAFE
       */
      bool push_with_possible_resize(thunk_type&& thunk)
      {
         assert(thunk);
         if(is_done_.load(std::memory_order_acquire)) return false;
         {
            for(bool did_push = false; !did_push;) {
               const auto offset = push_index_.fetch_add(1, std::memory_order_relaxed);

               // std::move<F> is not an error because try_push itself
               // will not forward unless it consumes the value.
               for(auto i = 0u; i != n_queues_ && !did_push; ++i) {
                  const auto [delta_capacity, success]
                      = queues_[(offset + i) % n_queues_].try_push_with_resize(std::move(thunk));
                  if(success) {
                     did_push = true;
                     if(delta_capacity > 0) note_increased_capacity_(delta_capacity);
                  } else {
                     assert(delta_capacity == 0);
                  }
               }
            }
         }
         increment_size_();
         return true;
      }

      /**
       * @brief Attempt to pop an element; non-blocking.
       * @return `true` if, and only if, an element is popped.
       *
       * THREAD SAFE
       */
      bool try_pop(thunk_type& thunk)
      {
         assert(!thunk);
         if(try_pop_(thunk)) {
            assert(thunk);
            return true;
         }
         return false;
      }

      /**
       * @brief Pops an element, blocking if there's no element
       * @return `true` if, and only if, an element is popped. Only returns
       * `false` when `signal_done()` has been called _and_ the queue is
       * empty.
       *
       * The calling thread will block if the queue is empty.
       *
       * THREAD SAFE
       */
      bool blocking_pop(thunk_type& thunk)
      {
         assert(!thunk);

         auto is_done      = [this]() { return is_done_.load(std::memory_order_acquire); };
         auto is_empty     = [this]() { return size_.load(std::memory_order_acquire) == 0; };
         auto stop_waiting = [this]() {
            return size_.load(std::memory_order_acquire) > 0
                   || is_done_.load(std::memory_order_acquire);
         };

         if(is_done() && is_empty()) return false;

         while(true) {
            // If we're done, and empty, then return FALSE
            // Note that once `is_done_` is set, no more thunks are pushed
            if(is_done()) {
               std::unique_lock lock{data_padlock_}; // no notify until after the lock
               if(is_empty()) { return false; }
            }

            if(try_pop_(thunk)) { break; }

            {
               std::unique_lock lock{data_padlock_};
               // What `wait_for` with a tight timeout?
               // The `stop_waiting` predicate relies on `size_`. To avoid a race
               // condition on `size_`, we'd have to lock `data_padlock_` before
               // updating it. That, however, creates a global lock whenever a job
               // gets pushed or popped -- but this thread pool is optimistically
               // lock free. So we have a short wait in case -- due to a race -- we
               // miss the data_cv_.notify when size is incremented.
               data_cv_.wait_for(lock, std::chrono::microseconds{10}, stop_waiting);
            }
         }

         return true;
      }
   };

} // namespace detail

// ------------------------------------------------------------------ ThreadPool

/**
 * A threadpool thread sets this id to the pointer to the owning
 * ThreadPool::Pimpl, so that a thread can check if it's running within
 * the pool.
 */
static thread_local uintptr_t this_thread_threadpool_id{0};

struct ThreadPool::Pimpl
{
   detail::NotificationQueue Q_;
   std::vector<std::thread> threads_;

   enum State : int { RUNNING, DONE };
   std::atomic<int> state_{RUNNING};

 private:
   void run_f(thunk_type& f)
   {
      // Execute function
      assert(f);
      try {
         f();
      } catch(...) {
         FATAL("uncaught exception in thread-pool, should never happen");
      }
   }

   void run_(unsigned thread_number)
   {
      this_thread_threadpool_id = pool_id();
      while(!Q_.done_is_signalled()) {
         while(try_run_one()) {
            continue; // try again!
         }

         {
            thunk_type f; // try a locking-pop
            {
               bool success = Q_.blocking_pop(f);
               if(!success)
                  break; // Only fails if queue has done signal, and Q is
                         // empty
            }

            run_f(f);
         }
      }
   }

 public:
   Pimpl(unsigned thread_count, unsigned n_queues, unsigned queue_capacity)
       : Q_(n_queues, queue_capacity)
   {
      threads_.reserve(thread_count);
      for(auto i = 0u; i < thread_count; ++i) threads_.emplace_back([this, i] { this->run_(i); });
   }
   ~Pimpl() { dispose(); }

   void dispose()
   {
      const auto old_state
          = State(std::atomic_exchange_explicit(&state_, int(DONE), std::memory_order_acq_rel));
      if(old_state == DONE) return;

      Q_.signal_done();
      for(auto& thread : threads_) {
         try {
            thread.join(); // join the threads
         } catch(std::system_error& e) {
            FATAL("system error joining thread: {}", e.what());
         }
      }
   }

   State state() const { return State(state_.load(std::memory_order_acquire)); }

   uintptr_t pool_id() const { return reinterpret_cast<uintptr_t>(this); }

   bool is_threadpool_thread() const { return this_thread_threadpool_id == pool_id(); }

   void post(thunk_type&& f, Policy policy)
   {
      if(state() == DONE || !f) return;
      switch(policy) {
      case Policy::BLOCK_WHEN_FULL:
         FATAL("not implemented"); // complicates with capacity
         break;
      //    Q_.blocking_push(std::move(f)); // may block
      //    break;
      case Policy::DISPATCH_WHEN_FULL:
         if(is_threadpool_thread()) {
            Q_.non_blocking_push(std::move(f)); // may execute a thunk on this thread
            break;
         }
         [[fallthrough]];
      case Policy::NEVER_BLOCK:
         Q_.push_with_possible_resize(std::move(f)); // could bad_alloc
         break;
      }
   }

   bool try_run_one()
   {
      if(state() == DONE) return false;
      thunk_type f;
      if(Q_.try_pop(f)) {
         run_f(f);
         return true;
      }
      return false;
   }
};

/**
 * @brief Construct a `ThreadPool` with `thread_count` concurrent executing
 *        threads. Backing storage is made up of `n_queue` queues, each
 *        with capacity `queue_capacity`, making the total capacity:
 *       `total capacity = n_queues * queue_capacity`.
 *
 * @param thread_count Number of concurrent threads. If zero, then
 *                     `std::hardward_concurrency()`.
 * @param n_queues Number of backing lock-free queues, used for storing
 *                 pending thunks. If zero, then `2 * thread_count` is used.
 * @param queue_capacity The capacity of each backing queue. If zero, then
 *                       the default capacity `256` is used.
 *
 * Exceptions:
 * + `std::bad_alloc`
 * + `std::system_error`  if, for some reason, threads cannot be created
 */
ThreadPool::ThreadPool(unsigned thread_count, unsigned n_queues, unsigned queue_capacity)
    : pimpl_(nullptr)
{
   if(thread_count == 0) thread_count = std::thread::hardware_concurrency();
   if(n_queues == 0) n_queues = 2 * thread_count;
   if(queue_capacity == 0) queue_capacity = 256;
   pimpl_.reset(new Pimpl(thread_count, n_queues, queue_capacity));
}

ThreadPool::~ThreadPool() = default;

/**
 * @brief Finishes tasks in queue, and then join underlying threads.
 *        Prevents new jobs from being posted.
 */
void ThreadPool::dispose() { pimpl_->dispose(); }

/**
 * @brief Posts thunk `f` to the queue.
 */
void ThreadPool::post(thunk_type f) { pimpl_->post(std::move(f), Policy::DISPATCH_WHEN_FULL); }

/**
 * @brief Posts thunk `f` to the queue, executing it immediately if already on a threadpool thread
 */
void ThreadPool::dispatch(thunk_type f)
{
   if(pimpl_->is_threadpool_thread()) {
      try {
         f(); // And execute the thunk
      } catch(...) {
         FATAL("logic error: exception in thunk");
      }
   } else {
      post(std::move(f));
   }
}

/**
 * @brief Posts thunk `f` to the queue, never executing immediately
 */
void ThreadPool::defer(thunk_type f) { pimpl_->post(std::move(f), Policy::NEVER_BLOCK); }

/**
 * @brief Attempt to dequeue a task, and run it. Non-blocking.
 * @return `true` if a task is dequeued and run.
 */
bool ThreadPool::try_run_one() { return pimpl_->try_run_one(); }

ThreadPoolExecutor ThreadPool::get_executor() { return ThreadPoolExecutor{*this}; }

} // namespace niggly::async
