
#pragma once

#include <system_error>

/**
 * @defgroup error-codes Error Codes
 * @ingroup niggly-utils
 *
 * ~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * // Out of memory!
 * return make_error_code(ecode::out_of_memory);
 * ~~~~~~~~~~~~~~~~~~~~~~
 */

namespace niggly
{
using std::error_code;

/**
 * @ingroup error-codes
 * @brief Complete set of niggly error codes.
 */
enum class ecode : int {
   okay = 0,           //!< i.e., everything's okay.
   out_of_memory,      //!< Equivalent to `std::bad_alloc`.
   system_error,       //!< Faulty logic in the program.
   logic_error,        //!< Faulty logic in the program.
   buffer_underflow,   //!< Attempt to read from an empty buffer.
   buffer_overflow,    //!< Attempt to write beyond the end of a buffer.
   index_out_of_range, //!< Index error.
   operation_failed,   //!< Like a system error, but within the program.
   exception_occurred, //!< Exception caught and forwarded as an error_code.
   argument_error,     //!< An invalid argument was supplied.
   type_error,         //!< Operation violates type system.
   uninitialized,      //!< Some state is unitialized.

   stream_not_ready, //!< I/O stream is not ready.
   premature_eof,    //!< I/O stream encountered premature end-of-file.
   fail,             //!< I/O stream set the `fail` bit.
   bad,              //!< I/O stream set the `bad` bit.
   object_too_large, //!< Attempt to read/write an object that is too large.
   invalid_data      //!< Input data (file/network/etc.) was invalid.
};
} // namespace niggly

namespace std
{
template<> struct is_error_code_enum<niggly::ecode> : true_type
{};
} // namespace std

namespace niggly
{
error_code make_error_code(ecode);
} // namespace niggly
