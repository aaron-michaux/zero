
#include "niggly/utils.hpp"

namespace niggly::example
{
struct Config
{
   bool show_help{false};
   string filename{""};
};

int cli_main(int argc, char** argv)
{
   Config config;
   auto has_error = false;

   for(int i = 1; i < argc; ++i) {
      string arg = argv[i];
      try {
         if(arg == "-h" || arg == "--help") {
            config.show_help = true;
         } else if(arg == "-f") {
            config.filename = cli::safe_arg_str(argc, argv, i);
         } else {
            cout << format("unexpected argument: '{}'", arg) << endl;
            has_error = true;
         }

      } catch(std::runtime_error& e) {
         cout << format("Error on command-line: {}", e.what()) << endl;
         has_error = true;
      }
   }

   if(has_error) {
      cout << format("aborting...") << endl;
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}
} // namespace niggly::example
