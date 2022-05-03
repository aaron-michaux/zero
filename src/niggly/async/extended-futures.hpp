
#pragma once

#include "stdinc.hpp"

#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>

namespace niggly::async
{
template<typename R> class Promise;
template<typename R> class Future;
template<typename R, typename... Args> class PackagedTask;

namespace detail
{

   template<typename R>
   class PromiseFutureSharedState : public std::enable_shared_from_this<PromiseFutureSharedState<R>>
   {
    public:
      enum class Status : int8_t { UNSET, SET, CANCELLED };
      static constexpr bool is_void_result = std::is_same_v<R, void>;

    private:
      using ResultType = std::conditional_t<is_void_result, int8_t, R>;

      mutable std::mutex padlock_       = {};
      std::condition_variable cv_       = {};
      std::exception_ptr exception_ptr_ = {};
      std::optional<ResultType> value_  = {}; //
      std::atomic<Status> status_       = Status::UNSET;
      bool future_is_retreived_         = false;
      std::function<void()> then_       = {};

      Status load_status_() const { return status_.load(std::memory_order_acquire); }
      void store_status_locked_(Status value) { status_.store(value, std::memory_order_release); }

      template<typename WaitFunc> Status wait_with_thunk_(WaitFunc&& thunk)
      {
         if(promise_is_unset()) {
            std::unique_lock<std::mutex> lock{padlock_};
            while(promise_is_unset()) { thunk(lock); }
         }
         const auto status = load_status_();
         std::atomic_thread_fence(std::memory_order_acq_rel);
         return status;
      }

      void notify_locked_(Status new_status)
      {
         store_status_locked_(new_status);
         cv_.notify_all();
         if(then_) then_(); // execute the continuation
      }

      void flag_future_has_been_retreived_locked_()
      {
         if(future_is_retreived_)
            throw std::future_error{std::future_errc::future_already_retrieved};
         future_is_retreived_ = true;
      }

    public:
      virtual ~PromiseFutureSharedState() = default;

      ///@{ getters
      bool promise_is_unset() const { return load_status_() == Status::UNSET; }
      bool promise_is_set() const { return load_status_() == Status::SET; }
      bool is_cancelled() const { return load_status_() == Status::CANCELLED; }
      bool is_ready() const { return promise_is_set(); }
      ///@}

      void flag_future_has_been_retreived()
      {
         std::lock_guard lock{padlock_};
         flag_future_has_been_retreived_locked_();
      }

      void cancel()
      {
         std::lock_guard lock{padlock_};
         notify_locked_(Status::CANCELLED);
      }

      ///@{ the execption ptr
      bool has_exception_ptr() const
      {
         std::lock_guard lock{padlock_};
         return exception_ptr_ != nullptr;
      }

      void set_exception_ptr(std::exception_ptr ex_ptr)
      {
         std::lock_guard lock{padlock_};
         if(!promise_is_unset())
            throw std::future_error{std::future_errc::promise_already_satisfied};
         exception_ptr_ = ex_ptr;
         notify_locked_(Status::SET);
      }
      ///@}

      ///@{ wait
      Status wait()
      {
         return wait_with_thunk_([this](std::unique_lock<std::mutex>& lock) { cv_.wait(lock); });
      }

      template<typename Rep, typename Period>
      std::future_status wait_for(const std::chrono::duration<Rep, Period>& duration)
      {
         auto status = wait_with_thunk_([this, &duration](std::unique_lock<std::mutex>& lock) {
            cv_.wait_for(lock, duration);
         });
         return (status == Status::UNSET) ? std::future_status::timeout : std::future_status::ready;
      }

      template<typename Clock, typename Duration>
      std::future_status wait_until(const std::chrono::time_point<Clock, Duration>& deadline)
      {
         auto status = wait_with_thunk_([this, &deadline](std::unique_lock<std::mutex>& lock) {
            cv_.wait_until(lock, deadline);
         });
         return (status == Status::UNSET) ? std::future_status::timeout : std::future_status::ready;
      }
      ///@}

      ///@{ get/set value
      R get()
      {
         auto status = wait(); // noop if `is_set`
         if(status == Status::CANCELLED) throw std::future_error{std::future_errc::broken_promise};
         if(exception_ptr_ != nullptr) std::rethrow_exception(exception_ptr_);
         if constexpr(!is_void_result) {
            assert(value_.has_value());
            return std::move(*value_);
         }
      }

      template<typename T = R>
      std::enable_if_t<!std::is_same_v<T, void>, void> set_value(T&& new_value)
      {
         std::lock_guard lock{padlock_};
         auto status = load_status_();
         if(status == Status::SET)
            throw std::future_error{std::future_errc::promise_already_satisfied};
         else if(status == Status::CANCELLED)
            return; // do nothing
         value_ = std::forward<T>(new_value);
         notify_locked_(Status::SET);
      }

      template<typename T = R> std::enable_if_t<std::is_same_v<T, void>, void> set_value()
      {
         std::lock_guard lock{padlock_};
         auto status = load_status_();
         if(status == Status::SET)
            throw std::future_error{std::future_errc::promise_already_satisfied};
         else if(status == Status::CANCELLED)
            return; // do nothing
         notify_locked_(Status::SET);
      }
      ///@}

      ///@{ then
      template<typename ExecutionBroker, typename Allocator, typename F>
      Future<std::invoke_result_t<F, R>>
      then(const ExecutionBroker& executor, const Allocator& alloc, F&& f)
      {
         using T = std::invoke_result_t<F, R>;
         PackagedTask<T, R> work{std::allocator_arg_t{}, alloc, std::forward<F>(f)};
         auto continuation_future  = work.get_future(); // return result
         bool schedule_immediately = false;             // iff result is set

         // Create the continuation
         auto thunk = [shared_state = this->shared_from_this(),
                       work_state   = work.shared_state_]() mutable {
            switch(shared_state->load_status_()) {
            case Status::UNSET: // should *never* happen
               assert(false);
               break;
            case Status::SET: // do the work =)
               try {
                  if constexpr(is_void_result)
                     work_state->run();
                  else
                     work_state->run(shared_state->get());
               } catch(...) {
                  // propogate the exception
                  work_state->set_exception_ptr(std::current_exception());
               }
               break;
            case Status::CANCELLED: // propogate cancellation
               work_state->cancel();
               break;
            }
         };

         { // Safely test if we should execute immediately, or wait
            std::lock_guard lock{padlock_};
            schedule_immediately = !promise_is_unset();
            assert(!then_);
            then_ = [executor, thunk = std::move(thunk)]() { executor.execute(std::move(thunk)); };
         }

         // If set_value happens here, you are out of luck

         if(schedule_immediately) {
            assert(promise_is_set() || is_cancelled());
            // the future and/or exception has already been set, so `then` clause
            // should be executed immediately
            then_();
         } else {
            // then_() will be run whenever the Promise/PackagedTask associated with
            // `this_future` has its state set
         }

         return continuation_future;
      }
      ///@}
   };

   template<typename R, typename... Args>
   class TaskSharedStateInterface : public PromiseFutureSharedState<R>
   {
    public:
      virtual ~TaskSharedStateInterface() = default;
      virtual void run(Args&&... args)    = 0;
   };

   template<typename Fn, typename R, typename... Args>
   class TaskSharedState final : public TaskSharedStateInterface<R, Args...>

   {
    private:
      Fn f; // the function/lambda!

    public:
      template<typename Fn2>
      TaskSharedState(Fn2&& fn)
          : f{std::forward<Fn2>(fn)}
      {
         using F = typename std::decay_t<Fn2>;
         static_assert(std::is_invocable_v<F, Args...>);
      }

      void run(Args&&... args) override
      {
         try {
            run_impl(std::forward<Args>(args)...);
         } catch(...) {
            this->set_exception_ptr(std::current_exception());
         }
      }

    private:
      template<typename T = R>
      std::enable_if_t<std::is_same_v<T, void>, void> run_impl(Args&&... args)
      {
         auto& ref = f;
         std::invoke(ref, std::forward<Args>(args)...);
         this->set_value();
      }

      template<typename T = R>
      std::enable_if_t<!std::is_same_v<T, void>, void> run_impl(Args&&... args)
      {
         this->set_value(std::invoke(f, std::forward<Args>(args)...));
      }
   };

} // namespace detail

// ------------------------------------------------------------------------------------------ Future

/**
 * @ingroup async
 * @brief Provides a way to access the result of an asynchronous operation.
 *
 * Future is designed to work with Promise, PackagedTask, and async_thunk. Each provides
 * a Future object to access the result.
 *
 * It is possible to _blocking wait_ on the result, or alternatively set a `then` function
 * which will non-blocking execute when the value of the Future is set.
 *
 * To work with Promises, etc., Futures require some shared state stored on the heap. One
 * memory allocation is required -- at minimum -- every time a Promise-Future or
 * PackagedTask-Future pair is created. Allocators are supported for this reason.
 */
template<typename R> class Future final
{
 private:
   using shared_state_type = detail::PromiseFutureSharedState<R>;
   std::shared_ptr<shared_state_type> shared_state_{};

   void release_(); // release shared state

   friend class Promise<R>;
   template<typename> friend class detail::PromiseFutureSharedState;

   template<typename, typename...> friend class PackagedTask;

   // throws std::future_error
   // no_state/future_already_retrieved
   explicit Future(std::shared_ptr<shared_state_type> shared_state)
       : shared_state_{std::move(shared_state)}
   {
      shared_state_->flag_future_has_been_retreived();
   }

 public:
   ///@{ @name construction/assignment/swap
   Future() noexcept;
   Future(const Future& o)                = delete;
   Future(Future&& o) noexcept            = default; // Ensure shared state released
   ~Future()                              = default;
   Future& operator=(const Future& o)     = delete;
   Future& operator=(Future&& o) noexcept = default; // use swap
   /**
    * @brief A standard `noexcept` swap operation.
    */
   void swap(Future& o) noexcept
   {
      using std::swap;
      swap(shared_state_, o.shared_state_);
   }
   ///@}

   ///@{ @name getters
   /** @brief True iff the Future is still associated with some Promise or PackagedTask. */
   bool valid() const noexcept { return shared_state_ != nullptr; }

   /** @brief True iff the Future's value is set and can be retrieved without blocking. */
   bool is_ready() const noexcept { return valid() && shared_state_->is_ready(); }

   /** @brief True iff the Future has been cancelled. */
   bool is_cancelled() const noexcept { return valid() && shared_state_->is_ready(); }

   /** @brief True iff the Future contains an exception which will be thrown when calling get(). */
   bool has_exception() const
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      return shared_state_->has_exception_ptr();
   }
   ///@}

   /**
    * @brief A standard `noexcept` swap operation.
    */
   friend void swap(Future& a, Future& b) noexcept { a.swap(b); }

   ///@{ @name operations
   /**
    * @brief Release the shared state, making `valid() == false`.
    */
   void reset() noexcept { shared_state_.reset(); }

   /**
    * @brief Cancel the operation.
    *
    * Cancelling a future puts it in a `cancelled` state. (`is_cancelled() == true`.)
    * If the PackagedTask has not yet executed, it will not be.
    * The cancellation will propogate to any `then` clause if set.
    *
    * @exception std::future_error Throws a `future_errc::no_state` error if the internal shared
    * state has already been ejected.
    */
   void cancel()
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      shared_state_->cancel(); // will provent "work" from being done
   }

   /**
    * @brief Gets the result of the Future, performing a blocking wait if not yet set.
    *
    * This operation is _NOT_ thread safe: only one thread should attempt to call `get()`
    * on any given Future. The operation does, of course, sychronize with any other thread
    * that sets the result from the associated Promise or PackagedTask.
    *
    * Will never wait if `is_ready() == true` or `is_cancelled() == true`.
    *
    * Use a `then` clause if you want code to automatically run when the Future's value is
    * ready.
    *
    * @exception std::future_error Throws a `future_errc::no_state` error if the internal shared
    * state has already been ejected.
    * @exception ... Will rethrow any exception that occured during the compution of the Future's
    * value.
    */
   R get()
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      using std::swap;
      std::shared_ptr<shared_state_type> shared_state{nullptr};
      swap(shared_state_, shared_state); // release shared state
      return shared_state->get();
   }
   ///@}

   ///@{ @name wait
   /**
    * @brief Blocking wait until the Future's value is ready - or the Future is cancelled.
    *
    * @exception std::future_error Throws a `future_errc::no_state` error if the internal shared
    * state has already been ejected.
    */
   void wait() const
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      shared_state_->wait();
   }

   /**
    * @brief Blocking wait for the shorter of `duration`, or when the Future's value is
    * cancelled/ready.
    *
    * @exception std::future_error Throws a `future_errc::no_state` error if the internal shared
    * state has already been ejected.
    */
   template<typename Rep, typename Period>
   std::future_status wait_for(const std::chrono::duration<Rep, Period>& duration) const
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      return shared_state_->wait_for(duration);
   }

   /**
    * @brief Blocking wait up until `deadline`, or when the Future's value is cancelled/ready -
    * whichever is shorter.
    *
    * @exception std::future_error Throws a `future_errc::no_state` error if the internal shared
    * state has already been ejected.
    */
   template<typename Clock, typename Duration>
   std::future_status wait_until(const std::chrono::time_point<Clock, Duration>& deadline) const
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      return shared_state_->wait_until(deadline);
   }
   ///@}

   ///@{ @name then

   /**
    * @see then(const ExecutionBroker&, F&&)
    */
   template<typename ExecutionBroker, typename F>
   Future<std::invoke_result_t<F, R>> then(const ExecutionBroker& executor, F&& f)
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      return shared_state_->then(executor, std::allocator<int>{}, std::forward<F>(f));
   }

   /**
    * @brief Execute `f` when the associated Promise or PackagedTask sets the value
    * on the shared state.
    *
    * If the value is already set, then `f` is executed immediately.
    *
    * The ExecutionBroker is responsible for posting the task to some execution context (e.g.,
    * a thread pool), or executing immediately.
    *
    * If Allocator is specified, then it will be used to allocate the shared state for the
    * new PackagedTask-Future pair. Otherwise the standard library allocator is used.
    *
    * @return A Future for accessing the result of the `then` clause. Cancellation will
    * propagate to the returned Future.
    *
    * @exception std::future_error Throws a `future_errc::no_state` error if the internal shared
    * state has already been ejected.
    */
   template<typename ExecutionBroker,
            typename Allocator,
            typename F,
            typename T = std::invoke_result_t<F, R>>
   Future<T> then(const ExecutionBroker& executor, const Allocator& alloc, F&& f)
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      return shared_state_->then(executor, alloc, std::forward<F>(f));
   }
   ///@}
};

// ----------------------------------------------------------------------------------------- Promise

/**
 * @ingroup async
 * @brief Stores a value or exception for later asychronous retrieval.
 */
template<typename R> class Promise final
{
 private:
   using shared_state_type = detail::PromiseFutureSharedState<R>;
   std::shared_ptr<shared_state_type> shared_state_{};

 public:
   Promise()
       : shared_state_{std::make_shared<shared_state_type>()}
   {}
   template<class Alloc>
   Promise(std::allocator_arg_t, const Alloc& alloc)
       : shared_state_{std::allocate_shared<shared_state_type, Alloc>(alloc)}
   {}
   Promise(Promise&& o) noexcept { *this = std::move(o); }
   Promise(const Promise&) = delete;
   ~Promise() {}
   Promise& operator=(Promise&& o) noexcept { this->swap(o); }
   Promise& operator=(const Promise&) = delete;

   void swap(Promise& o) noexcept
   {
      using std::swap;
      swap(shared_state_, o.shared_state_);
   }
   friend void swap(Promise& a, Promise& b) noexcept { a.swap(b); }

   bool valid() const noexcept { return shared_state_ != nullptr; }

   void cancel()
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      std::shared_ptr<shared_state_type> shared_state{nullptr};
      swap(shared_state_, shared_state); // release shared state
      shared_state->cancel();
   }

   Future<R> get_future()
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      return Future<R>{shared_state_};
   }

   void reset() { shared_state_.reset(); }

   template<typename T = R> std::enable_if_t<!std::is_same_v<T, void>, void> set_value(T&& value)
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      shared_state_->set_value(std::forward<T>(value));
   }

   template<typename T = R> std::enable_if_t<std::is_same_v<T, void>, void> set_value()
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      shared_state_->set_value();
   }

   void set_exception(std::exception_ptr ex)
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      shared_state_->set_exception_ptr(ex);
   }
};

// ------------------------------------------------------------------------------------ PackagedTask

/**
 * @ingroup async
 * @brief Wrapps a Callable whose value or exception can be retrieved asychronously.
 */
template<typename R, typename... Args> class PackagedTask final
{
 private:
   using shared_state_type = detail::TaskSharedStateInterface<R, Args...>;
   std::shared_ptr<shared_state_type> shared_state_{};

   template<typename> friend class detail::PromiseFutureSharedState;

   void set_exception_ptr_(std::exception_ptr ex_ptr)
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      shared_state_->set_exception_ptr(ex_ptr);
   }

 public:
   ///@{ @name construction/assignment/swap
   PackagedTask() noexcept = default;

   template<typename F>
   explicit PackagedTask(F&& f)
       : shared_state_{std::make_shared<detail::TaskSharedState<F, R, Args...>>(std::forward<F>(f))}
   {}

   template<typename F, typename Allocator>
   explicit PackagedTask(std::allocator_arg_t, const Allocator& alloc, F&& f)
       : shared_state_{std::allocate_shared<detail::TaskSharedState<F, R, Args...>, Allocator>(
           alloc,
           std::forward<F>(f))}
   {}

   /**
    * @brief Copy-constructible so that PackagedTask can be copied into a std::function
    */
   PackagedTask(const PackagedTask&) = default;

   PackagedTask(PackagedTask&& o) noexcept { *this = std::move(o); }

   ~PackagedTask() = default;

   PackagedTask& operator=(const PackagedTask&) = default;

   PackagedTask& operator=(PackagedTask&& o) noexcept
   {
      this->swap(o);
      return *this;
   }

   /**
    * @brief A standard `noexcept` swap operation.
    */
   void swap(PackagedTask& o) noexcept
   {
      using std::swap;
      swap(shared_state_, o.shared_state_);
   }
   ///@}

   /**
    * @brief A standard `noexcept` swap operation.
    */
   friend void swap(PackagedTask& a, PackagedTask& b) noexcept { a.swap(b); }

   ///@{ @name getters
   /** @brief True iff the Packaged is still associated with some state. */
   bool valid() const noexcept { return shared_state_ != nullptr; }

   /** @brief True iff the PackagedTask has been cancelled. */
   bool is_cancelled() const noexcept { return valid() && shared_state_->is_ready(); }

   /** @brief True iff the PackagedTask has already been `set`. */
   bool is_ready() const noexcept { return valid() && shared_state_->is_ready(); }

   /** @brief True iff the PackagedTask is holding an exception. */
   bool has_exception() const
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      return shared_state_->has_exception_ptr();
   }
   ///@}

   ///@{ @name operations
   /**
    * @brief Release the shared state, making `valid() == false`.
    */
   void reset() { shared_state_.reset(); }

   /**
    * @brief Cancel the operation.
    *
    * Cancelling a PackagedTask puts it in a `cancelled` state. (`is_cancelled() == true`.)
    * Anything waiting on the associated Future is immediately notified, and `then` clauses
    * are automatically run. The cancellation will propogate into the `then `claues.
    *
    * If the PackagedTask has not yet executed, it will not be.
    *
    * @exception std::future_error Throws a `future_errc::no_state` error if the internal shared
    * state has already been ejected.
    */
   void cancel()
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      std::shared_ptr<shared_state_type> shared_state{nullptr};
      using std::swap;
      swap(shared_state_, shared_state); // release shared state
      shared_state->cancel();
   }

   /**
    * @brief Gets the Future associated with this result of this PackagedTask.
    *
    * @exception std::future_error Throws a `future_errc::no_state` error if the internal shared
    * state has already been ejected.
    *
    * @exception std::future_error Throws a `future_errc::future_already_retrieved` error if the
    * future has already been retreived.
    */
   Future<R> get_future()
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      return Future<R>{shared_state_};
   }

   /**
    * @brief Calls the stored task with args as the arguments.
    *
    * The return value of the task or any
    * exceptions thrown are stored in the shared state. The shared state is made ready and any
    * threads waiting for this are unblocked.
    *
    * Exceptions
    * + `std::future_error` with code `std::future_errc::no_state` if the internal shared state has
    *    already been ejected. state has already been ejected.
    * + `std::future_error` with code `future_errc::promise_already_satisfied` if the result has
    *    already been set. result has already been set.
    *
    * @param args the parameters to pass on invocation of the stored task
    * @return (none)
    */
   void operator()(Args... args)
   {
      if(!valid()) throw std::future_error{std::future_errc::no_state};
      if(shared_state_->is_cancelled()) return; // do nothing
      shared_state_->run(std::forward<Args>(args)...);
   }
   ///@}
};

// -------------------------------------------------------------------------------------------- when

template<typename Executor, typename Allocator, typename R, typename F>
Future<std::invoke_result_t<F, R>>
when(Future<R>& future, const Executor& executor, const Allocator& alloc, F&& f)
{
   return future.then(executor, alloc, std::forward<F>(f));
}

} // namespace niggly::async
