
#pragma once

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

#include "error-codes.hpp"
#include "timestamp.hpp"

/**
 * @defgroup niggly-io Input/Output
 * @ingroup niggly-utils
 */

namespace niggly
{
// -------------------------------------------------------------- Some Utilities
/// @ingroup niggly-io
/// @brief Returns true if hardware natively uses `ieee754` float/doubles
constexpr bool is_cpu_ieee754_packing();

uint32_t pack_f32(float x);  // assumes 1, 8, 23
uint64_t pack_f64(double x); // assumes 1, 11, 52
float unpack_f32(uint32_t x);
double unpack_f64(uint64_t x);

// Reading and writing never more accurate than
// std::numeric_limits<T>::epsilon()
// Accuracy is worse if hardware doesn't use
// 8-bit exponent, 23-bit fraction (float)
// and 11-bit exponent, 52-bit fraction (double)

// ----------------------------------------------------------- Fundamental Types

error_code write_bool(std::ostream& out, bool x);
error_code write_i8(std::ostream& out, int8_t x);
error_code write_i16(std::ostream& out, int16_t x);
error_code write_i32(std::ostream& out, int32_t x);
error_code write_i64(std::ostream& out, int64_t x);
error_code write_u8(std::ostream& out, uint8_t x);
error_code write_u16(std::ostream& out, uint16_t x);
error_code write_u32(std::ostream& out, uint32_t x);
error_code write_u64(std::ostream& out, uint64_t x);
error_code write_f32(std::ostream& out, float x);
error_code write_f64(std::ostream& out, double x);

error_code write(std::ostream& out, bool x);
error_code write(std::ostream& out, int8_t x);
error_code write(std::ostream& out, int16_t x);
error_code write(std::ostream& out, int32_t x);
error_code write(std::ostream& out, int64_t x);
error_code write(std::ostream& out, uint8_t x);
error_code write(std::ostream& out, uint16_t x);
error_code write(std::ostream& out, uint32_t x);
error_code write(std::ostream& out, uint64_t x);
error_code write(std::ostream& out, float x);
error_code write(std::ostream& out, double x);

error_code read_bool(std::istream& in, bool& x);
error_code read_i8(std::istream& in, int8_t& x);
error_code read_i16(std::istream& in, int16_t& x);
error_code read_i32(std::istream& in, int32_t& x);
error_code read_i64(std::istream& in, int64_t& x);
error_code read_u8(std::istream& in, uint8_t& x);
error_code read_u16(std::istream& in, uint16_t& x);
error_code read_u32(std::istream& in, uint32_t& x);
error_code read_u64(std::istream& in, uint64_t& x);
error_code read_f32(std::istream& in, float& x);
error_code read_f64(std::istream& in, double& x);

error_code read(std::istream& in, bool& x);
error_code read(std::istream& in, int8_t& x);
error_code read(std::istream& in, int16_t& x);
error_code read(std::istream& in, int32_t& x);
error_code read(std::istream& in, int64_t& x);
error_code read(std::istream& in, uint8_t& x);
error_code read(std::istream& in, uint16_t& x);
error_code read(std::istream& in, uint32_t& x);
error_code read(std::istream& in, uint64_t& x);
error_code read(std::istream& in, float& x);
error_code read(std::istream& in, double& x);

// ----------------------------------------------------------- String/raw buffer

error_code write(std::ostream& out, std::string_view x);
error_code write(std::ostream& out, const std::vector<char>& x);
error_code write(std::ostream& out, const char* raw, uint32_t sz);
error_code write_big(std::ostream& out, const char* raw, std::size_t sz);
error_code read(std::istream& in, std::string& x);
error_code read(std::istream& in, std::vector<char>& x);

// ------------------------------------------------------------ "advanced" types

error_code write(std::ostream& in, const Timestamp& o);
error_code read(std::istream& in, Timestamp& o);

} // namespace niggly
