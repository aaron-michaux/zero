
/**
 * Read a single file in, and output "make" rules
 * 1. If produces a module
 * 2. The modules it depends on
 * Note, doing this right, doing headers, requires a macro preprocessor
 *
 * Syntax of note:
 * import core.speech; // Import a module
 * import <iostream>; // Import a system "header unit"

 * Module names: (note: export, and module cannot be used in a module name)
 * module-name:
 *    [module-name-qualifier] identifier ;
 * module-name-qualifier:
 *    identifier "." |
 *    module-name-qualifier identifier "." ;
 *
 * module-declaration:
 *    ["export"] "module" module-name [module-partition] [attribute-specifier-seq] ";" ;
 * module-partition:
 *    ":" module-name ;
 *
 * All imports appear before any delcarations
 */

/**
 * main.o: gcm.cache/module.c++m iostream.c++m cstdlib.c++m
 * 
 * speech.o gcm.cache/speech.gcm: speech.ccp speech:spanish.c++m speech:english.c++m
 * speech.c++m: gcm.cache/speech.gcm
 * .PHONY speech.c++m
 * gcm.cache/speech.gcm:| speech.o
 * 
 * speech_english.o gcm.cache/speech-english.gcm: speech_english.cpp
 * speech:english.c++m: gcm.cache/speech-english.gcm
 * .PHONY speech:english.c++m
 * gcm.cache/speech-english.gcm:| speech_english.o
 * 
 */

#include <cstdlib>
#include <string>
#include <string_view>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <array>

#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <ctre/ctre.hpp>

// -------------------------------------------------------------------------- Declarations

using std::array;
using std::string;
using std::string_view;
using std::cout;
using std::cerr;
using std::endl;
using std::runtime_error;
using std::vector;
using fmt::format;

const std::array cpp_headers = { "<any>", "<bitset>", "<chrono>", "<compare>", "<csetjmp>", "<csignal>", "<cstdarg>", "<cstdlib>", "<ctime>", "<functional>", "<initializer_list>", "<optional>", "<source_location>", "<stacktrace>", "<tuple>", "<type_triats>", "<typeindex>", "<typeinfo>", "<utility>", "<variant>", "<version>", "<memory>", "<memory_resource>", "<new>", "<scoped_allocator>", "<cfloat>", "<cinttypes>", "<climts>", "<cstdint>", "<limits>", "<cassert>", "<cerrno>", "<exception>", "<stdexcept>", "<system_error>", "<cctype>", "<charconv>", "<cstring>", "<cuchar>", "<cwchar>", "<cwctype>", "<format>", "<string>", "<string_view>", "<array>", "<deque>", "<forward_list>", "<list>", "<map>", "<queue>", "<set>", "<span>", "<stack>", "<unordered_map>", "<unordered_set>", "<vector>", "<iterator>", "<ranges>", "<algorithm>", "<execution>", "<bits>", "<cfenv>", "<cmath>", "<complex>", "<numbers>", "<numeric>", "<random>", "<ratio>", "<valarray>", "<clocale>", "<locale>", "<cstdio>", "<fstream>", "<iomanip>", "<ios>", "<iosfwd>", "<iostream>", "<istream>", "<ostream>", "<spanstream>", "<sstream>", "<streambuf>", "<strstream>", "<syncstream>", "<filesystem>", "<atomic>", "<barrier>", "<condition_variable>", "<future>", "<latch>", "<mutex>", "<semaphore>", "<shared_mutex>", "<stop_token>", "<thread>" };

struct Config
{
   string_view filename;
   string_view out_basedir;
   string_view moduledir = "gcm.cache";
   vector<string> include_paths;

   bool show_help = false;
   bool has_error = false;
};

void process_file(const Config& config);
void show_help(const char * argv0);
Config parse_command_line(int, const char* const*);

// -------------------------------------------------------------------------- process_file

void process_file(const Config& config)
{
   string_view filename = config.filename;
   string_view out_basedir = config.out_basedir;
   string_view moduledir = config.moduledir;
   string_view delim = (out_basedir.size() > 0 && out_basedir.back() == '/') ? "/" : "";

   const auto fpath = std::filesystem::path(filename);
   const auto extlen = strlen(fpath.extension().c_str());
   auto outfile_base = string_view(begin(filename),
                                   begin(filename) + (filename.size() - extlen));
   const string outfile = format("{}{}{}.o", out_basedir, delim, outfile_base);
   
   // State processing variables
   bool in_global_module_fragment = false;
   bool in_module_purview = false;
   bool is_module_export = false;
   bool found_processing_end = false;

   string module_name = "";
   vector<string> deps;
   vector<string> export_deps;
   
   // Strip comments from the line...
   bool in_c_comment = false; // The internal state of the 'comment stripper'
   auto strip_line_comments = [&in_c_comment] (string_view line, string& buffer) {
      const auto line_size = line.size();
      buffer.clear();
      buffer.reserve(line_size);
      string::value_type last_ch = -1;
      for(const auto ch: line) {
         if(in_c_comment) {
            if(last_ch == '*' && ch == '/') {
               in_c_comment = false;
            }
         } else {
            if(last_ch == '/' && ch == '/') { // C++ comment
               buffer.pop_back();             // remove last character
               break;                         // skip to end of line
            } else if(last_ch == '/' && ch == '*') { 
               in_c_comment = true;           // C comment
               buffer.pop_back();             // Remove last character
            } else {
               buffer.push_back(ch);
            }
         }
         last_ch = ch;
      }
      return string_view(std::begin(buffer), std::end(buffer));
   };
   
   
   // Process a single line (Does not support multi-line [C] comments)
   auto process_line = [&] (string_view line, const int lineno) {
      //cout << lineno << ": " << line << endl;

      // Do we have an empty line?
      if(ctre::match<"^\\s*$">(line)) {
         return;
      }
      
      // Do we have a preprocessor directive?
      if(ctre::match<"^\\s*#.*$">(line)) {
         // Discard preprocessor directives. Yes this is a bug.
         // Clang will hopefully have "scandeps" ready eventually
         return;
      }

      // Is this the start of the global module fragment?
      if(ctre::match<"^\\s*module\\s*;\\s*$">(line)) { 
         if(in_global_module_fragment || in_module_purview) 
            throw runtime_error(format("global module fragment at line #{} has invalid locations!",
                                       lineno));
         in_global_module_fragment = true;
         return;
      }
            
      // Do we have a module declaration?
      if(const auto match = ctre::match<"^\\s*export\\s+module\\s+([a-zA-Z0-9_\\.:]+)\\s*;\\s*$">(line); match) {
         in_global_module_fragment = false;
         in_module_purview = true;
         module_name = match.get<1>().to_string();
         is_module_export = true;
         return;
      }
      
      // Do we have a module implementation (without export)? 
      if(const auto match = ctre::match<"^\\s*module\\s+([a-zA-Z0-9_\\.:]+)\\s*;\\s*$">(line); match) {
         if(match.get<1>().to_view() == ":private") {
            // The beginning of the private module fragment... end processing
            found_processing_end = true;
            return;
         }
         
         in_global_module_fragment = false;
         in_module_purview = true;         
         module_name = match.get<1>().to_string();
         deps.push_back(std::move(module_name));
         return;
      }

      // Are we importing a module?
      if(const auto match = ctre::match<"^\\s*(export\\s+)?import\\s+([a-zA-Z0-9_\\.:\"<>]+)\\s*;\\s*$">(line); match) {
         const bool is_export_import = match.get<1>().to_view().size() > 0; // Affects dependencies
         // const bool is_header_fragment = match.get<2>().to_view().at(0) == '<'
         //    || match.get<2>().to_view().at(0) == '"';
         const bool is_partition = match.get<2>().to_view().at(0) == ':';
         if(is_partition && !in_module_purview) {
            throw runtime_error(format("attempt to import a module partition '{}' before the module declaration", match.get<2>().to_view()));
         }

         const string dependency = (is_partition)
            ? format("{}{}", module_name, match.get<2>().to_view())
            : match.get<2>().to_string();
         deps.push_back(dependency);
         if(is_export_import) {
            export_deps.push_back(dependency);
         }

         return;
      }

      {
         found_processing_end = true; // Don't bother looking further into the file
      }
   };
   
   { // Read the file line-by-line      
      std::ifstream infile(filename.data());
      string line;
      string stripped_line; // store an input line with comments stripped
      int lineno = 0;
      while (std::getline(infile, line) && !found_processing_end) {
         process_line(strip_line_comments(line, stripped_line), ++lineno);
      }
   }

   auto make_module_name = [&] (string_view module_name) -> string {
      string out_module = format("{0}/{1}.gcm", moduledir, module_name);
      for(auto& ch: out_module) if(ch == ':') ch = '-';
      return out_module;
   };

   auto is_cpp_header = [&] (string_view dep) {
      return dep.size() > 1 && dep.front() == '<'
         && std::find(begin(cpp_headers), end(cpp_headers), dep) != end(cpp_headers);
   };
   
   auto output_dependency = [&] (auto& os, string_view dep) {
      if(is_cpp_header(dep)) {
         os << ' ' << moduledir << "$(STDHDR_DIR)/"
            << string_view(begin(dep) + 1, end(dep) - 1) << ".gcm";
      } else {
         os << ' ' << make_module_name(dep);
      }
   };   

   const string outdep = format("$(BUILDDIR)/{0}", outfile);
   
   // Are we producing a module?
   if(is_module_export) {
      const auto out_module = make_module_name(module_name);
      cout << out_module << ": " << outdep << endl;
   }
   cout << outdep << ':';
   for(const auto& dependency: deps)
      output_dependency(cout, dependency);
   cout << endl << endl;
}

// ----------------------------------------------------------------------------- show_help

void show_help(const char *)
{
   cout << R"V0G0N(

   Usage: scandeps <filename>

)V0G0N";
}

// -------------------------------------------------------------------- parse_command_line

Config parse_command_line(int argc, const char* const* argv)
{
   Config conf;
   
   for(int i = 1; i < argc; ++i) {
      auto arg = string_view(argv[i]);
      if(arg == "-h" || arg == "--help") {
         conf.show_help = true;
         return conf; // Early return... no need to look at other switches
      }
   }

   for(int i = 1; i < argc; ++i) {
      auto arg = string_view(argv[i]);
      if(arg == "-d") {
         ++i;
         if(i < argc) {
            conf.out_basedir = argv[i];
         } else {
            cerr << "ERROR: must specify the output base after -d!" << endl;
            conf.has_error = true;
         }
      } else if(arg.starts_with("-isystem")) {
         conf.include_paths.emplace_back(begin(arg) + 8, end(arg));
      } else if(arg.starts_with("-I")) {
         conf.include_paths.emplace_back(begin(arg) + 2, end(arg));
      } else if(arg.starts_with("-")) {
         // ignore switch
      } else if(conf.filename.size() == 0) {
         conf.filename = arg;
      } else {
         cerr << "ERROR: attempt to set input file to '" << arg << "'"
              << " but it was already set to '" << conf.filename << "'"
              << endl;
         conf.has_error = true;
      }
   }

   if(conf.filename.size() == 0) {
      cerr << "ERROR: must specify a filename to process!" << endl;
      conf.has_error = true;
   } else if(!std::filesystem::exists(conf.filename)) {
      cerr << "ERROR: file not found '" << conf.filename << "'" << endl;
      conf.has_error = true;
   }
   
   return conf;
}

// ---------------------------------------------------------------------------------- main

int main(int argc, const char* const* argv)
{
   const auto conf = parse_command_line(argc, argv);
   if(conf.has_error) {
      cerr << "Aborting due to previous errors..." << endl;
      return EXIT_FAILURE;
   }
   if(conf.show_help) {
      show_help(argv[0]);
      return EXIT_SUCCESS;
   }

   try {
      process_file(conf);
   } catch(std::exception& e) {
      cerr << "Exception processing file '" << conf.filename << "': " << e.what() << endl;
      return EXIT_FAILURE;
   }
   
   return EXIT_SUCCESS;
}
