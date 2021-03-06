/******************************************************************/
/**
 * @file   sso-string.hpp
 * @author Elliot Goodrich
 * @author Aaron Michaux
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *********************************************************************/

#ifndef INCLUDE_GUARD_2EE24263_BD1F_4E5D_8CDA_A3217E867BF0
#define INCLUDE_GUARD_2EE24263_BD1F_4E5D_8CDA_A3217E867BF0

#include <algorithm>
#include <bit>
#include <climits>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <type_traits>

namespace sso23::detail
{
static constexpr std::size_t const high_bit_mask = static_cast<std::size_t>(1)
                                                   << (sizeof(std::size_t) * CHAR_BIT - 1);
static constexpr std::size_t const sec_high_bit_mask = static_cast<std::size_t>(1)
                                                       << (sizeof(std::size_t) * CHAR_BIT - 2);

template<typename T> constexpr unsigned char& most_sig_byte(T& obj)
{
   return *(std::bit_cast<unsigned char*>(&obj) + sizeof(obj) - 1);
}

template<int N> constexpr bool lsb(unsigned char byte) { return byte & (1u << N); }

template<int N> constexpr bool msb(unsigned char byte) { return byte & (1u << (CHAR_BIT - N - 1)); }

template<int N> constexpr void set_lsb(unsigned char& byte, bool bit)
{
   if(bit) {
      byte |= 1u << N;
   } else {
      byte &= ~(1u << N);
   }
}

template<int N> constexpr void set_msb(unsigned char& byte, bool bit)
{
   if(bit) {
      byte |= 1u << (CHAR_BIT - N - 1);
   } else {
      byte &= ~(1u << (CHAR_BIT - N - 1));
   }
}
} // namespace sso23::detail

namespace sso23
{
/**
 * @ingroup niggly-strings
 * @brief A string type that supports short string optimization
 *
 * @todo Allocator
 * @todo string::buffer object that can release it's internal buffer... for vformat
 * @todo erase, insert, assign
 */
template<typename CharT, typename Traits = std::char_traits<CharT>> class basic_string
{
   using UCharT = typename std::make_unsigned<CharT>::type;

   constexpr basic_string(CharT const* string, std::size_t size, std::size_t capacity)
   {
      if(capacity <= sso_capacity) {
         Traits::move(m_data.sso.string, string, size);
         Traits::assign(m_data.sso.string[size], static_cast<CharT>(0));
         set_sso_size(size);
      } else {
         m_data.non_sso.ptr = new CharT[capacity + 1];
         Traits::move(m_data.non_sso.ptr, string, size);
         Traits::assign(m_data.non_sso.ptr[size], static_cast<CharT>(0));
         set_non_sso_data(size, capacity);
      }
   }

 public:
   using traits_type = Traits;
   using value_type  = CharT;

   constexpr basic_string() noexcept
       : basic_string{"", static_cast<std::size_t>(0)}
   {}

   constexpr basic_string(CharT const* string, std::size_t size)
       : basic_string(string, size, size)
   {}

   constexpr basic_string(std::string_view sv)
       : basic_string{sv.data(), sv.size()}
   {}

   template<std::input_iterator InputIt>
   constexpr basic_string(InputIt first, InputIt last)
       : basic_string{std::string_view{first, last}}
   {}

   constexpr basic_string(CharT const* string)
       : basic_string{string, std::strlen(string)}
   {}

   constexpr explicit basic_string(std::size_t size, CharT ch)
       : basic_string{}
   {
      resize(size, ch);
   }

   constexpr basic_string(const basic_string& string)
   {
      if(string.sso()) {
         m_data.sso = string.m_data.sso;
      } else {
         new(this) basic_string{string.data(), string.size()};
      }
   }

   constexpr basic_string(basic_string&& string) noexcept
   {
      m_data = string.m_data;
      string.set_moved_from();
   }

   constexpr basic_string& operator=(basic_string const& other)
   {
      auto copy = other;
      swap(copy, *this);
      return *this;
   }

   constexpr basic_string& operator=(basic_string&& other) noexcept
   {
      this->~basic_string();
      m_data = other.m_data;
      other.set_moved_from();
      return *this;
   }

   constexpr ~basic_string()
   {
      if(!sso()) { delete[] m_data.non_sso.ptr; }
   }

   constexpr CharT* data() noexcept { return sso() ? m_data.sso.string : m_data.non_sso.ptr; }

   constexpr CharT const* data() const noexcept
   {
      return sso() ? m_data.sso.string : m_data.non_sso.ptr;
   }

   constexpr std::size_t size() const noexcept
   {
      if(sso()) {
         return sso_size();
      } else {
         return read_non_sso_data().first;
      }
   }

   constexpr std::size_t capacity() const noexcept
   {
      if(sso()) {
         return sizeof(m_data) - 1;
      } else {
         return read_non_sso_data().second;
      }
   }

   constexpr void resize(std::size_t new_size, CharT value = static_cast<CharT>(0))
   {
      const auto old_size = size();
      if(new_size > capacity()) reserve(std::max(new_size, std::size_t(capacity() * 2.1)));
      if(sso())
         set_sso_size(new_size);
      else
         set_non_sso_data(new_size, capacity());
      auto ptr = data();
      for(size_t i = old_size; i < new_size; ++i) Traits::assign(ptr[i], value);
      Traits::assign(ptr[new_size], static_cast<CharT>(0));
   }

   constexpr void reserve(std::size_t new_capacity)
   {
      const auto old_capacity = capacity();
      if(new_capacity > old_capacity && new_capacity > sso_capacity) {
         // Create a new string an move it!
         auto new_string = basic_string{data(), size(), new_capacity};
         *this           = std::move(new_string);
      }
   }

   constexpr CharT& at(int idx) { return at(std::size_t(idx)); }
   constexpr CharT& at(std::size_t idx)
   {
      if(idx >= size()) throw std::out_of_range{"index out of range"};
      return data()[idx];
   }
   constexpr const CharT& at(int idx) const { return at(std::size_t(idx)); }
   constexpr const CharT& at(std::size_t idx) const
   {
      if(idx >= size()) throw std::out_of_range{"index out of range"};
      return data()[idx];
   }

   constexpr CharT& operator[](int idx) noexcept { return data()[idx]; }
   constexpr CharT& operator[](std::size_t idx) noexcept { return data()[idx]; }
   constexpr const CharT& operator[](int idx) const noexcept { return data()[idx]; }
   constexpr const CharT& operator[](std::size_t idx) const noexcept { return data()[idx]; }

   friend constexpr void swap(basic_string& lhs, basic_string& rhs)
   {
      std::swap(lhs.m_data, rhs.m_data);
   }

   constexpr void push_back(CharT ch) { resize(size() + 1, ch); }

   constexpr auto begin() const noexcept { return data(); }
   constexpr auto cbegin() const noexcept { return data(); }
   constexpr auto rbegin() const noexcept { return std::make_reverse_iterator(end()); }
   constexpr auto crbegin() const noexcept { return std::make_reverse_iterator(cend()); }

   constexpr auto end() const noexcept { return data() + size(); }
   constexpr auto cend() const noexcept { return data() + size(); }
   constexpr auto rend() const noexcept { return std::make_reverse_iterator(begin()); }
   constexpr auto crend() const noexcept { return std::make_reverse_iterator(cbegin()); }

 private:
   constexpr void set_moved_from() noexcept { set_sso_size(0); }

   // We are using sso if the last two bits are 0
   constexpr bool sso() const noexcept
   {
      return !detail::lsb<0>(m_data.sso.size) && !detail::lsb<1>(m_data.sso.size);
   }

   // good
   constexpr void set_sso_size(unsigned char size) noexcept
   {
      m_data.sso.size = static_cast<UCharT>(sso_capacity - size) << 2;
   }

   // good
   constexpr std::size_t sso_size() const noexcept
   {
      return sso_capacity - ((m_data.sso.size >> 2) & 63u);
   }

   constexpr void set_non_sso_data(std::size_t size, std::size_t capacity)
   {
      auto& size_hsb           = detail::most_sig_byte(size);
      auto const size_high_bit = detail::msb<0>(size_hsb);

      auto& cap_hsb               = detail::most_sig_byte(capacity);
      auto const cap_high_bit     = detail::msb<0>(cap_hsb);
      auto const cap_sec_high_bit = detail::msb<1>(cap_hsb);

      detail::set_msb<0>(size_hsb, cap_sec_high_bit);

      cap_hsb <<= 2;
      detail::set_lsb<0>(cap_hsb, cap_high_bit);
      detail::set_lsb<1>(cap_hsb, !size_high_bit);

      m_data.non_sso.size     = size;
      m_data.non_sso.capacity = capacity;
   }

   constexpr std::pair<std::size_t, std::size_t> read_non_sso_data() const
   {
      auto size     = m_data.non_sso.size;
      auto capacity = m_data.non_sso.capacity;

      auto& size_hsb = detail::most_sig_byte(size);
      auto& cap_hsb  = detail::most_sig_byte(capacity);

      // Remember to negate the high bits
      auto const cap_high_bit     = detail::lsb<0>(cap_hsb);
      auto const size_high_bit    = !detail::lsb<1>(cap_hsb);
      auto const cap_sec_high_bit = detail::msb<0>(size_hsb);

      detail::set_msb<0>(size_hsb, size_high_bit);

      cap_hsb >>= 2;
      detail::set_msb<0>(cap_hsb, cap_high_bit);
      detail::set_msb<1>(cap_hsb, cap_sec_high_bit);

      return std::make_pair(size, capacity);
   }

 private:
   union Data
   {
      struct NonSSO
      {
         CharT* ptr;
         std::size_t size;
         std::size_t capacity;
      } non_sso;
      struct SSO
      {
         CharT string[sizeof(NonSSO) / sizeof(CharT) - 1];
         UCharT size;
      } sso;
   } m_data;

 public:
   static constexpr std::size_t const sso_capacity
       = sizeof(typename Data::NonSSO) / sizeof(CharT) - 1;
};

template<typename CharT, typename Traits>
constexpr bool operator==(const basic_string<CharT, Traits>& lhs, const CharT* rhs) noexcept
{
   return !std::strcmp(lhs.data(), rhs);
}

template<typename CharT, typename Traits>
constexpr bool operator==(const CharT* lhs, const basic_string<CharT, Traits>& rhs) noexcept
{
   return rhs == lhs;
}

template<typename CharT, typename Traits>
constexpr bool operator==(const basic_string<CharT, Traits>& lhs,
                          const basic_string<CharT, Traits>& rhs) noexcept
{
   if(lhs.size() != rhs.size()) return false;
   return !std::strcmp(lhs.data(), rhs.data());
}

template<typename CharT, typename Traits>
std::ostream& operator<<(std::ostream& stream, const basic_string<CharT, Traits>& string)
{
   return stream << string.data();
}

namespace detail
{
   template<std::size_t N> struct static_string
   {
      char str[N]{};
      constexpr static_string(const char (&s)[N])
      {
         for(std::size_t i = 0; i < N; ++i) str[i] = s[i];
      }
   };
} // namespace detail

using string = basic_string<char>;

namespace literals
{
   template<detail::static_string s> constexpr auto operator""_s() { return string{s.str}; }
} // namespace literals

} // namespace sso23

#endif
