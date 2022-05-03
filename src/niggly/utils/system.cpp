
#include <array>
#include <cstdio>
#include <deque>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace niggly
{
std::pair<std::string, int> exec(std::string_view command)
{
   std::array<char, 128> buffer;
   std::deque<char> result;

   FILE* pipe = popen(command.data(), "r");
   if(!pipe) { throw std::runtime_error("popen() failed!"); }
   int exitcode = -1;

   try {
      while(!ferror(pipe) && !feof(pipe)) {
         const size_t count = fread(buffer.data(), 1, buffer.size(), pipe);
         result.insert(end(result), buffer.data(), buffer.data() + count);
      }
      exitcode = pclose(pipe);
   } catch(std::exception& e) {
      pclose(pipe);
      throw e;
   }

   return {{begin(result), end(result)}, exitcode};
}

} // namespace niggly
