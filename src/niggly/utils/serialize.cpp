
#include "serialize.hpp"

#include "base-include.hpp"

#include "detail/ieee754-packing.hpp"

#include <boost/endian/conversion.hpp>

#include <cmath>
#include <cstdio>
#include <type_traits>

namespace niggly
{
// --------------------------------------------------------------------- Helpers

/// @private
template<typename T> static error_code check_ios_ready(T& ios)
{
   Expects(uint32_t(ios.exceptions()) == 0);
   if(ios.rdstate() != std::ios_base::goodbit)
      return make_error_code(ecode::stream_not_ready);
   return error_code();
}

/// @private
template<typename T> static error_code check_ios(T& ios)
{
   Expects(uint32_t(ios.exceptions()) == 0);
   const auto state = ios.rdstate();
   if(state == std::ios_base::goodbit) return error_code();

   if(state & std::ios_base::badbit) return make_error_code(ecode::bad);

   if((state & std::ios_base::eofbit) and (state & std::ios_base::failbit))
      return make_error_code(ecode::premature_eof);

   return make_error_code(ecode::fail);
}

// -------------------------------------------------------------- Some Utilities

constexpr bool is_cpu_ieee754_packing() { return detail::is_cpu_ieee754_packing(); }

/// @ingroup niggly-io
uint32_t pack_f32(float x) { return detail::pack_float(x); }

/// @ingroup niggly-io
uint64_t pack_f64(double x) { return detail::pack_float(x); }

/// @ingroup niggly-io
float unpack_f32(uint32_t x) { return detail::unpack_float(x); }

/// @ingroup niggly-io
double unpack_f64(uint64_t x) { return detail::unpack_float(x); }

// -------------------------------------------------------------------- integers
/// @private
template<typename T> error_code write_intT(std::ostream& out, T x)
{
   try {
      auto ec = check_ios_ready(out);
      if(ec) return ec;

      // to little endian
      boost::endian::native_to_little_inplace(x);
      out.write(reinterpret_cast<char*>(&x), sizeof(x));
      return check_ios(out);
   } catch(...) {
      return make_error_code(ecode::exception_occurred);
   }
}

/// @private
template<typename T> error_code read_intT(std::istream& in, T& x)
{
   try {
      auto ec = check_ios_ready(in);
      if(ec) return ec;

      std::array<char, 8> buffer;
      static_assert(sizeof(x) <= buffer.size());

      in.read(&buffer[0], sizeof(x));
      std::memcpy(&x, &buffer[0], sizeof(x)); // gracefully handle alignment
      boost::endian::little_to_native_inplace(x);
      return check_ios(in);
   } catch(...) {
      return make_error_code(ecode::exception_occurred);
   }
}

/// @ingroup niggly-io
error_code write_bool(std::ostream& out, bool x) { return write_intT(out, int8_t(x)); }

/// @ingroup niggly-io
error_code write_i8(std::ostream& out, int8_t x) { return write_intT(out, x); }

/// @ingroup niggly-io
error_code write_i16(std::ostream& out, int16_t x) { return write_intT(out, x); }

/// @ingroup niggly-io
error_code write_i32(std::ostream& out, int32_t x) { return write_intT(out, x); }

/// @ingroup niggly-io
error_code write_i64(std::ostream& out, int64_t x) { return write_intT(out, x); }

/// @ingroup niggly-io
error_code write_u8(std::ostream& out, uint8_t x) { return write_intT(out, x); }

/// @ingroup niggly-io
error_code write_u16(std::ostream& out, uint16_t x) { return write_intT(out, x); }

/// @ingroup niggly-io
error_code write_u32(std::ostream& out, uint32_t x) { return write_intT(out, x); }

/// @ingroup niggly-io
error_code write_u64(std::ostream& out, uint64_t x) { return write_intT(out, x); }

/// @ingroup niggly-io
error_code write_f32(std::ostream& out, float x) { return write_u32(out, pack_f32(x)); }

/// @ingroup niggly-io
error_code write_f64(std::ostream& out, double x) { return write_u64(out, pack_f64(x)); }

/// @ingroup niggly-io
error_code write(std::ostream& out, bool x) { return write_bool(out, x); }

/// @ingroup niggly-io
error_code write(std::ostream& out, int8_t x) { return write_i8(out, x); }

/// @ingroup niggly-io
error_code write(std::ostream& out, int16_t x) { return write_i16(out, x); }

/// @ingroup niggly-io
error_code write(std::ostream& out, int32_t x) { return write_i32(out, x); }

/// @ingroup niggly-io
error_code write(std::ostream& out, int64_t x) { return write_i64(out, x); }

/// @ingroup niggly-io
error_code write(std::ostream& out, uint8_t x) { return write_u8(out, x); }

/// @ingroup niggly-io
error_code write(std::ostream& out, uint16_t x) { return write_u16(out, x); }

/// @ingroup niggly-io
error_code write(std::ostream& out, uint32_t x) { return write_u32(out, x); }

/// @ingroup niggly-io
error_code write(std::ostream& out, uint64_t x) { return write_u64(out, x); }

/// @ingroup niggly-io
error_code write(std::ostream& out, float x) { return write_f32(out, x); }

/// @ingroup niggly-io
error_code write(std::ostream& out, double x) { return write_f64(out, x); }

/// @ingroup niggly-io
error_code read_bool(std::istream& in, bool& x)
{
   int8_t y      = 0;
   const auto ec = read_i8(in, y);
   if(!ec) x = y;
   return ec;
}

/// @ingroup niggly-io
error_code read_i8(std::istream& in, int8_t& x) { return read_intT(in, x); }

/// @ingroup niggly-io
error_code read_i16(std::istream& in, int16_t& x) { return read_intT(in, x); }

/// @ingroup niggly-io
error_code read_i32(std::istream& in, int32_t& x) { return read_intT(in, x); }

/// @ingroup niggly-io
error_code read_i64(std::istream& in, int64_t& x) { return read_intT(in, x); }

/// @ingroup niggly-io
error_code read_u8(std::istream& in, uint8_t& x) { return read_intT(in, x); }

/// @ingroup niggly-io
error_code read_u16(std::istream& in, uint16_t& x) { return read_intT(in, x); }

/// @ingroup niggly-io
error_code read_u32(std::istream& in, uint32_t& x) { return read_intT(in, x); }

/// @ingroup niggly-io
error_code read_u64(std::istream& in, uint64_t& x) { return read_intT(in, x); }

/// @ingroup niggly-io
error_code read_f32(std::istream& in, float& x)
{
   uint32_t i = 0;
   auto ec    = read_u32(in, i);
   if(!ec) x = unpack_f32(i);
   return ec;
}

/// @ingroup niggly-io
error_code read_f64(std::istream& in, double& x)
{
   uint64_t i = 0;
   auto ec    = read_u64(in, i);
   if(!ec) x = unpack_f64(i);
   return ec;
}

/// @ingroup niggly-io
error_code read(std::istream& in, bool& x) { return read_bool(in, x); }

/// @ingroup niggly-io
error_code read(std::istream& in, int8_t& x) { return read_i8(in, x); }

/// @ingroup niggly-io
error_code read(std::istream& in, int16_t& x) { return read_i16(in, x); }

/// @ingroup niggly-io
error_code read(std::istream& in, int32_t& x) { return read_i32(in, x); }

/// @ingroup niggly-io
error_code read(std::istream& in, int64_t& x) { return read_i64(in, x); }

/// @ingroup niggly-io
error_code read(std::istream& in, uint8_t& x) { return read_u8(in, x); }

/// @ingroup niggly-io
error_code read(std::istream& in, uint16_t& x) { return read_u16(in, x); }

/// @ingroup niggly-io
error_code read(std::istream& in, uint32_t& x) { return read_u32(in, x); }

/// @ingroup niggly-io
error_code read(std::istream& in, uint64_t& x) { return read_u64(in, x); }

/// @ingroup niggly-io
error_code read(std::istream& in, float& x) { return read_f32(in, x); }

/// @ingroup niggly-io
error_code read(std::istream& in, double& x) { return read_f64(in, x); }

// --------------------------------------------------------------------- strings
/// @ingroup niggly-io
/// must be picked up as std::string
error_code write(std::ostream& out, std::string_view x)
{
   if(x.size() > std::size_t(std::numeric_limits<std::streamsize>::max()))
      return make_error_code(ecode::object_too_large);
   return write(out, reinterpret_cast<const char*>(x.data()), uint32_t(x.size()));
}

/// @ingroup niggly-io
error_code write(std::ostream& out, const std::vector<char>& x)
{
   if(x.size() > std::numeric_limits<uint32_t>::max())
      return make_error_code(ecode::object_too_large);
   return write(out, &x[0], uint32_t(x.size()));
}

/// @ingroup niggly-io
/// must be picked up as vector<char>
error_code write(std::ostream& out, const char* raw, uint32_t sz)
{
   auto ec = write_u32(out, sz);
   if(ec) return ec;
   try {
      out.write(raw, int32_t(sz));
      return check_ios(out);
   } catch(...) {
      return make_error_code(ecode::exception_occurred);
   }
}

/// @ingroup niggly-io
error_code write_big(std::ostream& out, const char* raw, std::size_t sz)
{
   if(sz > std::size_t(std::numeric_limits<std::streamsize>::max()))
      return make_error_code(ecode::object_too_large);
   auto ec = write_u64(out, sz);
   if(ec) return ec;
   try {
      out.write(raw, std::streamsize(sz));
      return check_ios(out);
   } catch(...) {
      return make_error_code(ecode::exception_occurred);
   }
}

/**
 * @private
 */
template<typename T> inline error_code read_vec_type(std::istream& in, T& x)
{
   uint32_t sz = 0;
   auto ec     = read_u32(in, sz);
   if(ec) return ec;
   try {
      x.resize(sz); // TODO, should we have a limit here?
      in.read(x.data(), int32_t(sz));
      return check_ios(in);
   } catch(...) {
      return make_error_code(ecode::exception_occurred);
   }
}

/// @ingroup niggly-io
error_code read(std::istream& in, std::string& x) { return read_vec_type(in, x); }

/// @ingroup niggly-io
error_code read(std::istream& in, std::vector<char>& x) { return read_vec_type(in, x); }

// ------------------------------------------------------------ "advanced" types

/// @ingroup niggly-io
error_code write(std::ostream& in, const Timestamp& o)
{
   error_code ec = {};
   try {
      ec = write(in, str(o));
   } catch(...) {
      return make_error_code(ecode::exception_occurred);
   }
   return ec;
}

/// @ingroup niggly-io
error_code read(std::istream& in, Timestamp& o)
{
   string s;
   auto ec = read(in, s);
   if(!ec) {
      try {
         o.parse(s);
      } catch(...) {
         return make_error_code(ecode::invalid_data);
      }
   }
   return ec;
}

} // namespace niggly
