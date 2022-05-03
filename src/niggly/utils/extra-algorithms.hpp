
#pragma once

#include <range/v3/all.hpp>

#include <algorithm>

/**
 * @defgroup algorithms Algorithms
 * @ingroup niggly-utils
 *
 * Extra algorithms
 */
namespace niggly
{
/**
 * @ingroup algorithms
 * @brief sort-unique a range.
 */
template<typename T>
requires ranges::range<T>
void sort_unique(T& container)
{
   std::sort(begin(container), end(container));
   container.erase(std::unique(begin(container), end(container)), end(container));
}

/**
 * @ingroup algorithms
 * @brief `true` if `object` can be found in `range`.
 */
template<typename T>
requires ranges::range<T>
bool is_in(const auto& object, T& range)
{
   auto ii = std::find(cbegin(range), cend(range), object);
   return ii != cend(range);
}

// ----------------------------------------------------------------- Starts with
/**
 * @ingroup algorithms
 * @brief Returns `true` if `input` begins with `match`.
 */
template<class Range>
requires ranges::range<Range>
constexpr bool starts_with(const Range& input, const Range& match)
{
   return input.size() >= match.size() and std::equal(cbegin(match), cend(match), input.begin());
}

/**
 * @ingroup algorithms
 * @brief Returns `true` if `input` ends with `match`.
 */
template<class Range>
requires ranges::range<Range>
constexpr bool ends_with(const Range& input, const Range& match)
{
   return starts_with(ranges::reverse(input), ranges::reverse(match));
}

/**
 * @ingroup algorithms
 * @brief Test if a snippet of code can be run at compile time.
 * @include is-constexpr-friendly_ex.cpp
 */
template<class F, int = (F{}(), 0)> constexpr bool is_constexpr_friendly(F) { return true; }
constexpr bool is_constexpr_friendly(...) { return false; }

} // namespace niggly
