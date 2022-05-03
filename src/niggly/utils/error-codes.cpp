
#include "error-codes.hpp"

namespace niggly
{
namespace
{
   /**
    * @private
    */
   struct ECodeCategory : std::error_category
   {
      const char* name() const noexcept override;
      std::string message(int ev) const override;
   };

   /**
    * @private
    */
   const char* ECodeCategory::name() const noexcept { return "ecode"; }

   /**
    * @private
    */
   std::string ECodeCategory::message(int e) const
   {
      switch(static_cast<ecode>(e)) {
      case ecode::okay: return "okay";
      case ecode::out_of_memory: return "out of memory";
      case ecode::system_error: return "system error";
      case ecode::logic_error: return "logic error";
      case ecode::buffer_underflow: return "buffer underflow";
      case ecode::buffer_overflow: return "buffer overflow";
      case ecode::index_out_of_range: return "index out of range";
      case ecode::operation_failed: return "operation failed";
      case ecode::exception_occurred: return "exception occurred";
      case ecode::argument_error: return "argument error";
      case ecode::type_error: return "type error";
      case ecode::uninitialized: return "uninitialized";
      case ecode::stream_not_ready: return "stream not ready";
      case ecode::premature_eof: return "premature eof";
      case ecode::fail: return "fail";
      case ecode::bad: return "bad";
      case ecode::object_too_large: return "object too large";
      case ecode::invalid_data: return "invalid data";
      }
      return "(unknown error)";
   }

   /**
    * @private
    */
   static const ECodeCategory ecode_category{};
} // namespace

/**
 * @ingroup error-codes
 * @brief Make an `ecode` `std::error_code`.
 */
error_code make_error_code(ecode e) { return {static_cast<int>(e), ecode_category}; }

} // namespace niggly
