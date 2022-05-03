
#pragma once

#include <type_traits>

/**
 * @defgroup smart-pointers Smart Pointers
 * @ingroup niggly-utils
 */

namespace niggly
{
/**
 * @ingroup smart-pointers
 * @brief If a pointer is owned, but cannot be managed by unique_ptr/shared_ptr
 */
template<class T> using owned_ptr = T*;

/**
 * @ingroup smart-pointers
 * @brief Classes/structs should use `observer_ptr<T>` for cases where
 *        the class holds (or observes) a pointer, but does not own
 *        or manage the pointer.
 * @see https://en.cppreference.com/w/cpp/experimental/observer_ptr
 */
template<class W> class observer_ptr
{
 private:
   W* ptr_{nullptr};

 public:
   using element_type = W;
   using pointer      = W*;
   using reference    = typename std::add_lvalue_reference<W>::type;

   constexpr observer_ptr()
       : ptr_(nullptr)
   {}
   constexpr observer_ptr(std::nullptr_t)
       : ptr_(nullptr)
   {}
   explicit observer_ptr(element_type* ptr)
       : ptr_(ptr)
   {}
   template<class W2>
   observer_ptr(observer_ptr<W2> other)
       : ptr_(other.get())
   {}
   observer_ptr(const observer_ptr& other) = default;
   observer_ptr(observer_ptr&& other)      = default;
   ~observer_ptr()                         = default;

   observer_ptr& operator=(const observer_ptr& other) = default;
   observer_ptr& operator=(observer_ptr&& other)      = default;

   constexpr pointer release()
   {
      pointer p{ptr_};
      reset();
      return p;
   }

   std::strong_ordering operator<=>(const observer_ptr& o) const { return ptr_ <=> o.ptr_; }

   constexpr void swap(observer_ptr& other)
   {
      using std::swap;
      swap(ptr_, other.ptr_);
   }

   constexpr void reset(pointer p = nullptr) { ptr_ = p; }
   constexpr pointer get() const { return ptr_; }
   constexpr reference operator*() const { return *ptr_; }
   constexpr pointer operator->() const { return ptr_; }
   constexpr explicit operator bool() const { return ptr_ != nullptr; }
   constexpr explicit operator pointer() const { return ptr_; }

   friend observer_ptr<W> make_observer(W* p) { return observer_ptr<W>(p); }

   friend constexpr std::strong_ordering operator<=>(const observer_ptr& a, const observer_ptr& b)
   {
      return a.get() <=> b.get();
   }

   friend constexpr bool operator==(observer_ptr p, std::nullptr_t) { return !p; }

   friend constexpr bool operator==(std::nullptr_t, observer_ptr p) { return !p; }

   friend constexpr bool operator!=(observer_ptr p, std::nullptr_t) { return p.get() != nullptr; }

   friend constexpr bool operator!=(std::nullptr_t, observer_ptr p) { return p.get() != nullptr; }
};

} // namespace niggly

namespace std
{
template<class T> struct hash<::niggly::observer_ptr<T>>
{
   size_t operator()(::niggly::observer_ptr<T> p) { return hash<T*>()(p.get()); }
};

} // namespace std
