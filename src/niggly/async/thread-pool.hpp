/**
 * Copyright 2022 Aaron Michaux
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * @license MIT
 */

#pragma once

#include <functional>
#include <memory>

namespace niggly::async {

class ThreadPool;
class ThreadPoolExecutor;

/**
 * @ingroup async
 * @brief Thread pool executor, with fixed capacity backing storage.
 *        Non-blocking, unless the backing storage is full. In which case,
 *        the calling thread can either block, or dequeue a task and run it
 *        before enqueuing a new task.
 */
class ThreadPool {
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
  ThreadPool(unsigned thread_count = 0, unsigned n_queues = 0,
             unsigned queue_capacity = 0); // bad alloc
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = default;
  ~ThreadPool();
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool& operator=(ThreadPool&&) = default;

  std::size_t thread_count() const;

  void dispose();

  void post(thunk_type f);     // terminate on bad alloc
  void dispatch(thunk_type f); // terminate on bad alloc
  void defer(thunk_type f);    // terminate on bad alloc

  bool try_run_one(); // attempt to dequeue and run a task

  /**
   * @brief Steal and execute tasks from the pool, until stop_predicate returns true
   * If the task queue is empty, the current thread will sleep for 1 microsecond
   * @return The number of tasks executed before returning.
   */
  std::size_t steal_tasks_until(std::function<bool()> stop_predicate);

  /**
   * Returns an executor for executing on the thread pool
   */
  ThreadPoolExecutor get_executor();
};

/**
 * @ingroup async
 * @brief A wrapper around whatever form of execution agent you have.
 */
class ThreadPoolExecutor {
private:
  ThreadPool& pool_;

public:
  ThreadPoolExecutor(ThreadPool& pool) : pool_{pool} {}
  template <class F> void execute(F&& f) const noexcept { pool_.post(std::forward<F>(f)); }

  bool operator==(const ThreadPoolExecutor& o) const noexcept { return &pool_ == &o.pool_; }
};

} // namespace niggly::async
