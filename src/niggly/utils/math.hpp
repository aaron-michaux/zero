
#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <vector>

/**
 * @defgroup math Mathematical Functions
 * @ingroup niggly-utils
 */

namespace niggly {
// ---------------------------------------------------------------- NAN constant

constexpr double dNAN = std::numeric_limits<double>::quiet_NaN();
constexpr float fNAN = std::numeric_limits<float>::quiet_NaN();

// ---------------------------------------------------------------------- Random

class Random {
private:
  std::ranlux48_base gen_;
  std::uniform_real_distribution<double> uniform_dist_{0.0, 1.0};
  std::uniform_int_distribution<int> int_dist_;

public:
  void seed(uint32_t s) { gen_.seed(s); }
  double uniform() { return uniform_dist_(gen_); }
  int roll_die(int low, int high) {
    return int_dist_(gen_, decltype(int_dist_)::param_type{low, high});
  }
};

// ------------------------------------------------------------- weighted random
/**
 * @ingroup math
 * @brief Selects a weighted random element from a vector of weights
 * @param weights Vector of weights is modified during call.
 * @param g Random number generator to use.
 * @return A random index in `[0..N)`, where `N == weights.size()`
 */
template <typename T, typename Generator>
size_t weighted_random(std::vector<T>& weights, Generator& g) {
  static_assert(std::is_floating_point<T>::value,
                "weighted random template argument must be floating point");

  // Everything finite
  assert(std::all_of(cbegin(weights), cend(weights), [](auto val) { return std::isfinite(val); }));

  const size_t N = weights.size();

  if (N <= 1)
    return 0;

  { // what if there's a negative value? Set that to 0.0
    auto ii = std::min_element(begin(weights), end(weights));
    assert(ii != end(weights));
    if (*ii < T(0))
      for (auto& x : weights)
        x += *ii;
  }

  { // convert the weights to store accumulated values
    for (size_t i = 1; i < N; ++i) {
      assert(weights[i] >= T(0));
      weights[i] += weights[i - 1];
    }
  }

  { // randomly select one of them
    const T max_val = weights.back();
    if (max_val == T(0))
      return 0;

    std::uniform_real_distribution<T> uniform(T(0), max_val);
    const float roll = uniform(g);
    auto ii = std::upper_bound(cbegin(weights), cend(weights), roll);
    if (ii == cend(weights))
      return N - 1;
    return std::distance(cbegin(weights), ii);
  }
}

// ---------------------------------------------------------------------- modulo
/**
 * @ingroup math
 * @brief The modular arithemtic operator introduced by Gauss, and used in
 *        math. Differs from the `remainder` operator `%`.
 *
 * Modular arithemtic is arithmetic that "wraps around". For integers
 * `mod 5`, we have:
 *
 * ~~~~~~~~~~~~~~~~~
 * -8 -7 -6 | -5 -4 -3 -2 -1 | 0  1  2  3  4  | 5  6  7  8
 *  2  3  4 |  0  1  2  3  4 | 0  1  2  3  4  | 0  1  2  3
 * ~~~~~~~~~~~~~~~~~
 *
 * For example,
 *
 * ~~~~~~~~~~~~~~~~~ {.cpp}
 * modulo(-8, 5) == 2;
 * modulo( 7, 5) == 2;
 * ~~~~~~~~~~~~~~~~~
 */
template <typename T>
requires std::is_integral<T>::value constexpr T modulo(const T& a, const T& n) {
  assert(n > 0);
  if (a >= 0)
    return a % n;
  return n - (-a % n);
}

// ----------------------------------------------------------------------

// #define MATH_1ARG(name)                                                                            \
//   template <typename T>                                                                            \
//   requires std::is_floating_point<T>::value constexpr T name(T value) noexcept {                   \
//     if (std::is_constant_evaluated()) {                                                            \
//       return gcem::name(value);                                                                    \
//     } else {                                                                                       \
//       return std::name(value);                                                                     \
//     }                                                                                              \
//   }

// #define MATH_2ARG(name)                                                                            \
//   template <typename T>                                                                            \
//   requires std::is_floating_point<T>::value constexpr T name(T x, T y) noexcept {                  \
//     if (std::is_constant_evaluated()) {                                                            \
//       return gcem::name(x, y);                                                                     \
//     } else {                                                                                       \
//       return std::name(x, y);                                                                      \
//     }                                                                                              \
//   }

/**
 * @ingroup math
 * @brief Absolute value of a floating point or integer type, in consteval or runtime context.
 */
template <typename T>
requires std::is_arithmetic<T>::value constexpr T abs(T x) noexcept {
  if (std::is_constant_evaluated()) {
    return (x == T(0)) ? T(0) : x < T(0) ? -x : x;
  }
  if constexpr (std::is_floating_point<T>::value) {
    return std::fabs(x);
  } else {
    return std::abs(x);
  }
}

// MATH_1ARG(exp)
// MATH_1ARG(expm1)
// MATH_1ARG(log)
// MATH_1ARG(log2)
// MATH_1ARG(log10)
// MATH_1ARG(log1p)

// MATH_1ARG(ceil)
// MATH_1ARG(floor)
// MATH_1ARG(trunc)
// MATH_1ARG(sqrt)
// // MATH_1ARG(crbt)

// MATH_1ARG(erf)
// MATH_1ARG(tgamma)
// MATH_1ARG(lgamma)

// MATH_1ARG(sin)
// MATH_1ARG(cos)
// MATH_1ARG(tan)
// MATH_1ARG(asin)
// MATH_1ARG(acos)
// MATH_1ARG(atan)
// MATH_2ARG(atan2)

// MATH_1ARG(tanh)
// MATH_1ARG(cosh)
// MATH_1ARG(sinh)
// MATH_1ARG(atanh)
// MATH_1ARG(acosh)
// MATH_1ARG(asinh)

// #undef MATH_1ARG
// #undef MATH_2ARG

} // namespace niggly
