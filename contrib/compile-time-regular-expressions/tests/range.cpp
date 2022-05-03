#include <ctre.hpp>
#include <iostream>

static constexpr auto pattern = ctll::fixed_string("(?<first>[0-9])[0-9]++");

static constexpr auto name = ctll::fixed_string("first");

int main() {
	using namespace std::string_view_literals;
	auto input = "123,456,768"sv;

	#if CTRE_CNTTP_COMPILER_CHECK
		std::cout << "CNTTP supported\n";
	#else
		std::cout << "c++17 emulation\n";
	#endif

	#if CTRE_CNTTP_COMPILER_CHECK
	    for (auto match: ctre::range<"(?<first>[0-9])[0-9]++">(input)) {
	#else
		using namespace ctre::literals;
		
		for (auto match: ctre::range<pattern>(input)) {
	#endif
			
	#if CTRE_CNTTP_COMPILER_CHECK
			std::cout << std::string_view(match.get<"first">()) << "\n";
			std::cout << std::string_view(match.get<1>()) << "\n";
	#else
			std::cout << std::string_view(match.get<name>()) << "\n";
			std::cout << std::string_view(match.get<1>()) << "\n";
	#endif
		}
}
