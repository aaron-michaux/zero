
#pragma once

#include "base/pointer.hpp"
#include "error-codes.hpp"

#include <atomic>
#include <deque>

/**
 * @defgroup niggly-allocator Allocator
 * @ingroup niggly-utils
 */

namespace niggly
{
/**
 * @brief Custom mostly lock-free allocator for objects that don't require destruction
 * @ingroup niggly-allocator
 *
 * For (my) convenience, does not use the normal C++ allocator interface.
 * NOTE, this allocator is designed to work with sets of objects
 * that do not need to be destructed.
 *
 * The underlying page of memory is allocated to the template parameter
 * `k_cacheline_size`
 */
template<bool k_thread_safe           = true,
         std::size_t k_page_size      = 4096,
         std::size_t k_cacheline_size = 64>
class BasicLinearAllocator
{
 private:
   using position_type = std::conditional_t<k_thread_safe, std::atomic<uintptr_t>, uintptr_t>;

   struct Page
   {
    private:
      owned_ptr<void> memory  = nullptr;
      uintptr_t memory_end    = 0;
      position_type alloc_pos = 0;

      static uintptr_t read_position(const position_type& x) noexcept
      {
         if constexpr(k_thread_safe) {
            return x.load(std::memory_order_acquire);
         } else {
            return x;
         }
      }

      static void write_position(position_type& x, uintptr_t value) noexcept
      {
         if constexpr(k_thread_safe) {
            x.store(value, std::memory_order_release);
         } else {
            x = value;
         }
      }

    public:
      constexpr Page() { init(k_page_size); }
      constexpr Page(const Page&) = delete;
      constexpr Page(Page&& o) noexcept { *this = std::move(o); }
      constexpr ~Page() { std::free(memory); }

      constexpr Page& operator=(const Page&) = delete;
      constexpr Page& operator=(Page&& o) noexcept
      {
         using std::swap;
         clear();
         swap(memory, o.memory);
         swap(memory_end, o.memory_end);
         set_alloc_pos(read_position(o.alloc_pos));
         return *this;
      }

      constexpr void set_alloc_pos(uintptr_t pos)
      {
         if(memory == nullptr)
            assert(pos == 0);
         else
            assert(pos >= pos0() && pos <= memory_end);
         write_position(alloc_pos, pos);
      }

      constexpr void clear()
      {
         std::free(memory);
         memory     = nullptr;
         memory_end = 0;
         write_position(alloc_pos, 0);
      }

      uintptr_t pos0() const { return reinterpret_cast<uintptr_t>(memory); }

      constexpr void reset() { write_position(alloc_pos, pos0()); }

      constexpr void init(const size_t in_size) // throws std::bad_alloc
      {
         if(memory != nullptr) {
            if(in_size == size()) {
               reset();
               return; // Done.
            }
            clear();
         }

         // size must be an integral multiple of alignment
         const size_t size = (in_size < k_cacheline_size)
                                 ? k_cacheline_size
                                 : (in_size - (in_size % k_cacheline_size));
         assert(size % k_cacheline_size == 0);
         assert(size == k_cacheline_size || size <= in_size);

         // Grab memory
         memory = std::aligned_alloc(k_cacheline_size, size);

         // Did we fail?
         if(memory == nullptr) throw std::bad_alloc{};

         // Set internal members
         memory_end = pos0() + size;
         reset();
      }

      constexpr size_t size() const { return (memory_end == 0) ? 0 : size_t(memory_end - pos0()); }

      constexpr size_t n_bytes_allocated() const
      {
         const uintptr_t pos1 = read_position(alloc_pos);
         return std::min(pos1, memory_end) - pos0();
      }

      constexpr size_t n_bytes_remaining() const { return size() - n_bytes_allocated(); }

      constexpr bool out_of_memory() const
      {
         return (memory_end == 0) || (read_position(alloc_pos) == memory_end + 1);
      }

      constexpr void* alloc(const size_t bytes, const size_t align)
      {
         assert(align > 0);
         assert(memory != nullptr);
         assert(!(align & (align - 1))); // is a power of two

         void* ptr         = nullptr;
         uintptr_t new_pos = 0;
         uintptr_t pos     = read_position(alloc_pos);
         while(true) {
            const uintptr_t padding  = (1 + ~(pos & (align - 1))) & (~align);
            const uintptr_t to_alloc = bytes + padding;
            new_pos                  = pos + to_alloc;

            if constexpr(k_thread_safe) {
               if(alloc_pos.compare_exchange_weak(
                      pos, new_pos, std::memory_order_acq_rel, std::memory_order_relaxed)) {
                  ptr = reinterpret_cast<void*>(pos + padding);
                  assert(uintptr_t(ptr) % align == 0);
                  break;
               }
            } else {
               write_position(alloc_pos, new_pos);
               ptr = reinterpret_cast<void*>(pos + padding);
               assert(uintptr_t(ptr) % align == 0);
               break;
            }
         }

         if(new_pos > memory_end) { // allocation failed
            // Prevent wrapping of `alloc_pos`, leading to undefined behavior.
            // Store `memory-end + 1` to indicate out of memory condition
            write_position(alloc_pos, memory_end + 1);
            ptr = nullptr; // page should not be used again
         }

         return ptr;
      }
   };

   using page_type              = std::conditional_t<k_thread_safe, std::atomic<Page*>, Page*>;
   std::deque<Page*> old_pages_ = {};
   page_type page_              = nullptr;
   std::mutex padlock_          = {};

   Page* read_page()
   {
      if constexpr(k_thread_safe) {
         return page_.load(std::memory_order_acquire);
      } else {
         return page_;
      }
   }

   void write_page(Page* page)
   {
      if constexpr(k_thread_safe) {
         page_.store(page, std::memory_order_release);
      } else {
         page_ = page;
      }
   }

 public:
   ///@{ @name Construction/Destruction/Assignment
   constexpr BasicLinearAllocator()                            = default;
   constexpr BasicLinearAllocator(const BasicLinearAllocator&) = delete;
   constexpr BasicLinearAllocator(BasicLinearAllocator&&)      = delete;
   constexpr ~BasicLinearAllocator()
   {
      // Wink out the memory
      delete page_;
      for(auto old_page : old_pages_) delete old_page;
   }
   constexpr BasicLinearAllocator& operator=(const BasicLinearAllocator&) = delete;
   constexpr BasicLinearAllocator& operator=(BasicLinearAllocator&&)      = delete;
   ///@}

   ///@{ @name Constructing objects

   /// @brief Allocate a chunk of memory.
   constexpr void* alloc(const size_t bytes, const size_t align = 1)
   {
      Page* page_ptr = read_page();
      if(page_ptr != nullptr) {
         void* ptr = page_ptr->alloc(bytes, align);
         if(ptr != nullptr) return ptr;
      }

      auto thread_safe_version = [this](auto bytes, auto align) {
         std::lock_guard lock{padlock_};
         Page* page_ptr = read_page();
         if(page_ptr != nullptr) {
            void* ptr = page_ptr->alloc(bytes, align);
            if(ptr != nullptr) return ptr;
         }

         auto new_page_ptr_ = new Page;                           // create a new page
         void* ptr          = new_page_ptr_->alloc(bytes, align); // allocate on it
         if(page_ptr != nullptr) old_pages_.push_back(page_ptr);  // stash the current page
         write_page(new_page_ptr_);                               // set the new page

         if(ptr != nullptr) return ptr;
         throw std::bad_alloc{};
      };

      auto single_thread_version = [this](auto page_ptr, auto bytes, auto align) {
         auto new_page_ptr_ = new Page;                           // create a new page
         void* ptr          = new_page_ptr_->alloc(bytes, align); // allocate on it
         if(page_ptr != nullptr) old_pages_.push_back(page_ptr);  // stash the current page
         write_page(new_page_ptr_);                               // set the new page
         if(ptr != nullptr) return ptr;
         throw std::bad_alloc{};
      };

      if constexpr(k_thread_safe) {
         return thread_safe_version(bytes, align);
      } else {
         return single_thread_version(page_ptr, bytes, align);
      }
   }

   /// @brief Allocate and construct 1 or more objects whose alignment is
   ///        compatible with `alignof(Node)`. Object constructor must be
   ///        non-throwing.
   template<typename U> constexpr U* alloc_t(size_t count = 1)
   {
      static_assert(std::is_nothrow_constructible<U>::value);

      U* ptr = reinterpret_cast<U*>(alloc(sizeof(U) * count, alignof(U)));
      if(ptr == nullptr) return nullptr;
      assert(reinterpret_cast<uintptr_t>(ptr) % alignof(U) == 0);

      U* end = ptr + count;
      for(auto ii = ptr; ii != end; ++ii) new(ii) U{};
      return ptr;
   }
   ///@}
};

/**
 * @brief LinearAllocator compatible with C++ allocater concept
 */
template<typename T,
         bool k_thread_safe           = true,
         std::size_t k_page_size      = 4096,
         std::size_t k_cacheline_size = 64>
class CxxLinearAllocator
{
 public:
   using BaseAllocatorType  = BasicLinearAllocator<k_thread_safe, k_page_size, k_cacheline_size>;
   using value_type         = T;
   using pointer            = T*;
   using const_pointer      = const T*;
   using void_pointer       = void*;
   using const_void_pointer = const void*;
   using size_type          = std::size_t;
   using difference_type    = std::ptrdiff_t;
   using propagate_on_container_move_assignment = std::true_type;
   template<typename U> struct rebind
   {
      using other = CxxLinearAllocator<U, k_thread_safe, k_page_size, k_cacheline_size>;
   };
   template<typename, bool, std::size_t, std::size_t> friend class CxxLinearAllocator;

 private:
   std::shared_ptr<BaseAllocatorType> alloc_;

 public:
   constexpr CxxLinearAllocator()
       : alloc_{std::make_shared<BaseAllocatorType>()}
   {}
   template<typename U>
   constexpr CxxLinearAllocator(
       const CxxLinearAllocator<U, k_thread_safe, k_page_size, k_cacheline_size>& o) noexcept
       : alloc_{o.alloc_}
   {}
   constexpr CxxLinearAllocator(const CxxLinearAllocator&)                = default;
   constexpr CxxLinearAllocator(CxxLinearAllocator&&) noexcept            = default;
   constexpr ~CxxLinearAllocator()                                        = default;
   constexpr CxxLinearAllocator& operator=(const CxxLinearAllocator&)     = default;
   constexpr CxxLinearAllocator& operator=(CxxLinearAllocator&&) noexcept = default;

   constexpr pointer allocate(size_type count)
   {
      return static_cast<T*>(alloc_->alloc(count * sizeof(T), alignof(T)));
   }
   constexpr pointer allocate(size_type count, const_void_pointer hint) { return allocate(count); }
   constexpr void deallocate(pointer, size_type)
   { // no bookkeeping
   }

   constexpr size_type max_size() const
   {
      if constexpr(std::is_same<T, void>::value)
         return k_page_size;
      else
         return k_page_size / sizeof(T);
   }

   template<typename U, bool thread_safe, std::size_t page_size, std::size_t cacheline_size>
   constexpr bool
   operator==(const CxxLinearAllocator<U, thread_safe, page_size, cacheline_size>& o) noexcept
   {
      return alloc_.get() == o.alloc_.get();
   }

   template<typename U, bool thread_safe, std::size_t page_size, std::size_t cacheline_size>
   constexpr bool
   operator!=(const CxxLinearAllocator<U, thread_safe, page_size, cacheline_size>& o) noexcept
   {
      return !(*this == o);
   }
};

/**
 * @brief A thread safe linear allocator
 * @see https://nfrechette.github.io/2015/05/21/linear_allocator/
 */
template<typename T> using LinearAllocator = CxxLinearAllocator<T, true, 4096, 64>;

/**
 * @brief A single threaded linear allocator - allocs are essentially free
 * @see https://nfrechette.github.io/2015/05/21/linear_allocator/
 */
template<typename T> using SingleThreadLinearAllocator = CxxLinearAllocator<T, false, 4096, 64>;

} // namespace niggly
