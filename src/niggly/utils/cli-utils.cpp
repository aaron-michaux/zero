
#include "cli-utils.hpp"

#include <iterator>
#include <regex>

#include "base-include.hpp"

namespace niggly::cli
{
// ---------------------------------------------------------------- safe-arg-str
/**
 * @ingroup cli
 * @brief Get the argument after `i` from command line arguments `argc` and
 *        `argv`. `i` must be in the range `[0..argc)`. If `i+1 == argc`
 *        then an exception is thrown. Best illustrated by example.
 *
 * Preconditions:
 * + `argc` and `argv` describe an array of `char *` "c" strings.
 * + `i >= 0` and `i < argc`
 *
 * Postconditions:
 * + `i = i + 1`, i.e., ready to parse the next argument.
 *
 * Exceptions
 * + `std::runtime_error` if `i+1 >= argc`, or if string is too long to
 *    allocate.
 * + `std::bad_alloc` if allocation fails.
 */
std::string safe_arg_str(int argc, char** argv, int& i)
{
   Expects(argc >= 0);
   Expects(i >= 0 && i < argc);
   string out = {};
   try {
      {
         string arg = argv[i];
         ++i;

         if(i >= argc) {
            auto msg = fmt::format("expected string after argument '{}'", arg);
            throw std::runtime_error(msg);
         }
      }

      out = std::string(argv[i]);
   } catch(std::length_error& e) {
      throw std::runtime_error("length error creating string");
   }

   return out;
}

// ---------------------------------------------------------------- safe-arg-int
/**
 * @ingroup cli
 * @brief Parse the argument (as an integer) after `i` from command line
 *        arguments `argc` and `argv`. `i` must be in the range `[0..argc)`. If
 *        `i+1 == argc` then an exception is thrown. If argument cannot be
 *        parsed as a (possibly negative) integer, then an exception is
 *       thrown. Best illustrated by example.
 *
 * Preconditions:
 * + `argc` and `argv` describe an array of `char *` "c" strings.
 * + `i >= 0` and `i < argc`
 *
 * Postconditions:
 * + `i = i + 1`, i.e., ready to parse the next argument.
 *
 * Exceptions
 * + `std::runtime_error` if `i+1 >= argc` or if `argv[i+1]` cannot be
 *   parsed as an integer.
 */
int safe_arg_int(int argc, char** argv, int& i)
{
   Expects(argc >= 0);
   Expects(i >= 0 && i < argc);
   auto arg = argv[i];
   ++i;
   auto badness = (i >= argc);
   auto ret     = 0;

   if(!badness) {
      char* end     = nullptr;
      auto long_ret = strtol(argv[i], &end, 10);
      if(*end != '\0' or long_ret > std::numeric_limits<int>::max()
         or long_ret < std::numeric_limits<int>::lowest())
         badness = true;
      else
         ret = static_cast<int>(long_ret);
   }

   if(badness) {
      auto msg = fmt::format("expected integer after argument '{}'", arg);
      throw std::runtime_error(msg);
   }

   return ret;
}

// ------------------------------------------------------------- safe-arg-double
/**
 * @ingroup cli
 * @brief Parse the argument (as double) after `i` from command line arguments
 *        `argc` and `argv`. `i` must be in the range `[0..argc)`. If
 *        `i+1 == argc` then an exception is thrown. If argument cannot be
 *        parsed as a (possibly negative) integer, then an exception is
 *        thrown. Best illustrated by example.
 *
 * ~~~~~~~~~~~~~{.cpp}
 * Some code
 * ~~~~~~~~~~~~~
 *
 * Preconditions:
 * + `argc` and `argv` describe an array of `char *` "c" strings.
 * + `i >= 0` and `i < argc`
 *
 * Postconditions:
 * + `i = i + 1`, i.e., ready to parse the next argument.
 *
 * Exceptions:
 * + `std::runtime_error` if `i+1 >= argc` or if `argv[i+1]` cannot be
 *   parsed as a double.
 */
double safe_arg_double(int argc, char** argv, int& i)
{
   Expects(argc >= 0);
   Expects(i >= 0 && i < argc);
   auto arg = argv[i];
   ++i;
   auto badness = (i >= argc);
   auto ret     = 0.0;

   if(!badness) {
      char* end = nullptr;
      ret       = strtod(argv[i], &end);
      if(*end != '\0') badness = true;
   }

   if(badness) {
      auto msg = fmt::format("expected numeric after argument '{}'", arg);
      throw std::runtime_error(msg);
   }

   return ret;
}

// ------------------------------------------------------------------ parse args
/**
 * @ingroup cli
 * @brief Parses the passes string as if it were command-line arguments
 *        for a shell command. Returns the arguments as a vector of
 *        strings.
 */
std::vector<std::string> parse_cmd_args(const std::string_view ss)
{
   std::string line{ss};
   std::vector<std::string> args;
   const auto pattern = "('(\\'|[^'])*')|(\"(\\\"|[^\"])*\")|([\\S]+)";

   auto process = [](std::string& s) {
      Expects(s.size() >= 1);
      auto pos = 0u; // write
      auto end = unsigned(s.size() - 1);
      for(auto i = 1u; i < end; ++i) {
         auto ch = s[i];
         if(ch == '\\') {
            auto c2 = s[++i]; // guaranteed because of 'end'
            switch(c2) {
            case 'a': s[pos++] = '\a'; break;
            case 'b': s[pos++] = '\b'; break;
            case 'f': s[pos++] = '\f'; break;
            case 'n': s[pos++] = '\n'; break;
            case 'r': s[pos++] = '\r'; break;
            case 't': s[pos++] = '\t'; break;
            case 'v': s[pos++] = '\v'; break;
            case '\\': s[pos++] = '\\'; break;
            case '\'': s[pos++] = '\''; break;
            case '"': s[pos++] = '"'; break;
            default: s[pos++] = c2;
            }

            s[pos++] = s[++i]; // guaranteed because of 'end'
         } else {
            s[pos++] = ch;
         }
      }
      s.resize(pos);
   };

   ///@todo Use CTRE
   std::regex expr{pattern};
   auto words_begin = std::sregex_iterator(line.begin(), line.end(), expr);
   auto words_end   = std::sregex_iterator();
   while(words_begin != words_end) {
      auto s = (words_begin++)->str();

      bool is_squote = s.size() >= 2 and s.front() == '\'' and s.back() == '\'';
      bool is_dquote = s.size() >= 2 and s.front() == '"' and s.back() == '"';
      if(is_squote or is_dquote) process(s);

      args.push_back(std::move(s));
   }

   return args;
}

} // namespace niggly::cli
