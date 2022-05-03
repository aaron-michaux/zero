
#include "file-system.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

/**
 * @defgroup niggly-filesystem File System
 * @ingroup niggly-utils
 */
namespace niggly
{
// ----------------------------------------------------------- file-get-contents

/**
 * @private
 */
template<typename T> error_code file_get_contentsT(const std::string_view fname, T& data)
{
   std::unique_ptr<FILE, std::function<void(FILE*)>> fp(fopen(fname.data(), "rb"),
                                                        [](FILE* ptr) {
                                                           if(ptr) fclose(ptr);
                                                        });

   if(fp == nullptr) return make_error_code(std::errc(errno));

   if(fseek(fp.get(), 0, SEEK_END) == -1) {
      // Probably EBADF: stream was not seekaable
      return make_error_code(std::errc(errno));
   }

   auto fpos = ftell(fp.get());
   if(fpos == -1) {
      // Probably EBADF: stream was not seekaable
      return make_error_code(std::errc(errno));
   }

   auto sz = size_t(fpos < 0 ? 0 : fpos);

   try {
      data.reserve(sz);
      data.resize(sz);
   } catch(std::length_error& e) {
      return make_error_code(ecode::argument_error);
   } catch(std::bad_alloc& e) {
      return make_error_code(ecode::out_of_memory);
   } catch(...) {
      // some other error thrown by the allocator... shouldn't happen
      return make_error_code(ecode::exception_occurred);
   }

   if(fseek(fp.get(), 0, SEEK_SET) == -1) {
      // Probably should never happen
      return make_error_code(std::errc(errno));
   }

   if(data.size() != fread(&data[0], 1, data.size(), fp.get())) {
      if(ferror(fp.get()))
         return make_error_code(std::errc(errno));
      else if(feof(fp.get()))
         return make_error_code(ecode::premature_eof);
      else
         return make_error_code(ecode::operation_failed);
   }

   if(FILE* ptr = fp.release(); fclose(ptr) != 0) {
      return make_error_code(std::errc(errno));
   }

   return {};
}

/**
 * @ingroup niggly-filesystem
 * @brief Returns the file contents. See documentation for `file_get_contents()`
 */
std::string file_get_contents(const std::string_view fname)
{
   std::string out;
   auto ec = file_get_contents(fname, out);
   if(ec) throw std::runtime_error(ec.message());
   return out;
}

/**
 * @ingroup niggly-filesystem
 * @brief Reads the entire contents of `filename`, and returns it in `out`.
 *
 * Error codes:
 * + std::errc::not_enough_memory A call to malloc failed.
 * + Any error listed for the posix function `open()`.
 * + Any error generated by `fseek()`, `ftell()`, `fread()`, or `fclose()`.
 * + ecode::premature_eof The EOF indicator was set whilst reading.
 * + ecode::operation_failed Failed to read all the data, but `fread`
 *                           otherwise returned no error. Should not happen.
 * + ecode::out_of_memory Failed to allocate space for `out`.
 * + ecode::argument_error A `length error` was thrown when resizing `out`.
 * + ecode::exception_occured The allocator threw some other exception.
 *
 * @see [POSIX
 * open()](https://pubs.opengroup.org/onlinepubs/009695399/functions/open.html)
 */
error_code file_get_contents(const std::string_view filename, std::string& out)
{
   return file_get_contentsT(filename, out);
}

/**
 * @ingroup niggly-filesystem
 * @brief Overload to `file_put_contents(string_view, string)`.
 */
error_code file_get_contents(const std::string_view filename, std::vector<char>& out)
{
   return file_get_contentsT(filename, out);
}

// ----------------------------------------------------------- file-put-contents

/**
 * @ingroup niggly-filesystem
 * @brief Writes `dat` to file `filename`.
 *
 * Error codes:
 * + std::errc::not_enough_memory A call to malloc failed.
 * + Any of the errors listed for the posix function `open()`.
 * + Any of the errors generated by `fwrite()`.
 * + And of the errors generated by `fclose()`.
 * + ecode::premature_eof The EOF indicator was set whilst writing.
 * + ecode::operation_failed Failed to write all the data, but `fwrite`
 *                           otherwise returned no error. Should not happen.
 *
 * @see [POSIX
 * open()](https://pubs.opengroup.org/onlinepubs/009695399/functions/open.html)
 */
error_code file_put_contents(const std::string_view filename, const std::string_view dat)
{
   FILE* fp = fopen(filename.data(), "wb");
   if(fp == nullptr) return make_error_code(std::errc(errno));

   error_code ec = {};

   auto sz = fwrite(dat.data(), 1, dat.size(), fp);
   if(sz != dat.size()) {
      if(ferror(fp))
         ec = make_error_code(std::errc(errno));
      else if(feof(fp))
         ec = make_error_code(ecode::premature_eof);
      else
         ec = make_error_code(ecode::operation_failed);
   }
   if(fclose(fp) != 0)
      if(!ec) ec = make_error_code(std::errc(errno));

   return ec;
}

/**
 * @ingroup niggly-filesystem
 * @brief Overload to `file_put_contents(const string_view, const string_view)`.
 */
error_code file_put_contents(const std::string_view fname, const std::vector<char>& dat)
{
   return file_put_contents(fname, std::string_view(&dat[0], dat.size()));
}

// --------------------------------------------------- is-regular-file/directory

/**
 * @ingroup niggly-filesystem
 * @brief Returns `true` if the specified argument is a regular file.
 */
bool is_regular_file(const std::string_view filename)
{
   try {
      auto p = std::filesystem::path(filename);
      return std::filesystem::is_regular_file(p);
   } catch(...) {}
   return false;
}

/**
 * @ingroup niggly-filesystem
 * @brief Returns `true` if the specified argument is a directory.
 */
bool is_directory(const std::string_view filename)
{
   try {
      auto p = std::filesystem::path(filename);
      return std::filesystem::is_directory(p);
   } catch(...) {}
   return false;
}

// --------------------------------------------------- basename/dirname/file_ext

/**
 * @ingroup niggly-filesystem
 * @brief The absolute file path.
 */
std::string absolute_path(const std::string_view filename)
{
   std::string ret;
   try {
      namespace bf = std::filesystem;
      ret          = bf::absolute(bf::path(filename)).string();
   } catch(...) {}
   return ret;
}

/**
 * @ingroup niggly-filesystem
 * @brief Like the shell command `basename`, with the option of stripping the
 *        extension.
 */
std::string basename(const std::string_view filename, const bool strip_extension)
{
   std::string ret;
   try {
      namespace bf = std::filesystem;
      if(strip_extension)
         ret = bf::path(filename).stem().string();
      else
         ret = bf::path(filename).filename().string();
   } catch(...) {}
   return ret;
}

/**
 * @ingroup niggly-filesystem
 * @brief Like the shell command `dirname`.
 */
std::string dirname(const std::string_view filename)
{
   std::string ret;
   try {
      namespace bf = std::filesystem;
      ret          = bf::path(filename).parent_path().string();
   } catch(...) {}
   return ret;
}

/**
 * @ingroup niggly-filesystem
 * @brief Returns the file extension, something like `.png`
 */
std::string file_ext(const std::string_view filename)
{
   std::string ret;
   try {
      namespace bf = std::filesystem;
      ret          = bf::path(filename).extension().string();
   } catch(...) {}
   return ret;
}

std::string extensionless(const std::string_view filename)
{
   const auto ext = file_ext(filename);
   return std::string(filename.data(), filename.size() - ext.size());
}

// ----------------------------------------------------------------------- mkdir
/**
 * @ingroup niggly-filesystem
 * @brief Like the shell command `mkdir`, return `true` if successful.
 */
bool mkdir(const std::string_view dname)
{
   try {
      namespace fs = std::filesystem;
      if(fs::create_directory(fs::path(dname))) return true;
   } catch(...) {}
   return false;
}

/**
 * @ingroup niggly-filesystem
 * @brief Like the shell command `mkdir -p`, return `true` if successful.
 */
bool mkdir_p(const std::string_view dname)
{
   try {
      namespace fs = std::filesystem;
      if(fs::create_directories(fs::path(dname))) return true;
   } catch(...) {}
   return false;
}

} // namespace niggly
