
#pragma once

#include <fmt/format.h>

#include <array>
#include <chrono>
#include <limits>
#include <string>

#include <cassert>
#include <cstdint>
#include <ctime>

namespace niggly {
constexpr bool is_leap_year(int y);
constexpr bool is_valid_date(int y, int m, int d);
constexpr bool is_valid_time(int h, int m, int s);
constexpr int days_in_month(int y, int m); // [1..12]
constexpr int day_of_week(int y, int m, int d);
constexpr int day_of_year(int y, int m, int d);
constexpr bool seconds_to_tm(int64_t t, std::tm* tm);
constexpr int64_t datetime_to_seconds(int y, int m, int d, int H, int M, int S);

/**@brief Timestamp class with microsecond accuracy, based on 64 bit signed
 *        integer representation.
 * @ingroup date-time
 */
struct Timestamp {
public:
  using value_type = int64_t;

private:
  using system_clock_time_point = std::chrono::system_clock::time_point;

  static constexpr value_type M = 1000000;
  value_type x_{0};

public:
  ///@{ @name Construction/Destruction
  constexpr Timestamp() = default;
  constexpr explicit Timestamp(int64_t t, int micros) { set(t, micros); }
  constexpr explicit Timestamp(std::string_view s) { *this = parse(s); }
  constexpr Timestamp(int y, int m, int d, int hour = 0, int min = 0, int sec = 0, int micros = 0) {
    set(y, m, d, hour, min, sec, micros);
  }
  constexpr explicit Timestamp(value_type x) { x_ = x; }
  constexpr explicit Timestamp(std::chrono::system_clock::time_point whence)
      : x_{value_type(
            std::chrono::duration_cast<std::chrono::microseconds>(whence.time_since_epoch())
                .count())} {}
  constexpr Timestamp(const Timestamp&) = default;
  constexpr Timestamp(Timestamp&&) noexcept = default;
  constexpr ~Timestamp() = default;
  ///@}

  /**
   * @brief Parse a string as a timestamp.
   * Timestamp format is `YYYY-MM-DDtHH:MM:SS.micros`. The `t` can
   * be capiatlized. The clock time can use `:` or `-` for delimiters.
   * The micro seconds portion must be at most 6 characters long.
   *
   * It's valid to pass in:
   * + `YYYY-MM-DD`,
   * + `YYYY-MM-DDtHH:MM:SS`
   * + `YYYY-MM-DDtHH:MM:SS.[0-9]*`.
   */
  static constexpr Timestamp parse(std::string_view s) {
    // 0123456789.123456789.123456789.
    // 2018-04-11t21-35-56.356193

    if (s.size() < 10 or s.size() > 26) {
      throw std::invalid_argument(fmt::format("parse error: timestamp string "
                                              "'{}' length is {}, but must be "
                                              "10-26 characters long",
                                              s, s.size()));
    }

    // Sanity checks
    for (auto i = 0u; i < s.size(); ++i) {
      if (i == 4 or i == 7) {
        if (s[i] != '-')
          throw std::invalid_argument("unexpected character");
      } else if (i == 13 or i == 16) {
        if (s[i] != '-' and s[i] != ':')
          throw std::invalid_argument("unexpected character");
      } else if (i == 10) {
        if (s[i] != 't' and s[i] != 'T' and s[i] != ' ')
          throw std::invalid_argument("unexpected character");
      } else if (i == 19) {
        if (s[i] != '.')
          throw std::invalid_argument("unexpected character");
      }
    }

    auto d = [&](unsigned ind) -> int {
      if (ind >= s.size())
        return 0;
      if (!std::isdigit(s[ind]))
        throw std::invalid_argument("unexpected character");
      return int(s[ind] - '0');
    };

    int year = d(0) * 1000 + d(1) * 100 + d(2) * 10 + d(3);
    int month = d(5) * 10 + d(6);
    int day = d(8) * 10 + d(9);
    int hour = d(11) * 10 + d(12);
    int min = d(14) * 10 + d(15);
    int sec = d(17) * 10 + d(18);
    int micros = d(20) * 100000 + d(21) * 10000 + d(22) * 1000 + d(23) * 100 + d(24) * 10 + d(25);

    return Timestamp(year, month, day, hour, min, sec, micros);
  }

  ///@{ @name Special Timestamps
  /// @brief Calls `clock_gettime(CLOCK_REALTIME, ...)` to get system clock.
  static Timestamp now() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return Timestamp{ts.tv_sec, int(ts.tv_nsec / 1000)};
  }

  static constexpr Timestamp max() { return Timestamp{std::numeric_limits<value_type>::max()}; }
  static constexpr Timestamp min() { return Timestamp{std::numeric_limits<value_type>::lowest()}; }
  ///@}

  ///@{ @name Ordering
  constexpr bool operator==(const Timestamp& o) const { return x_ == o.x_; }
  constexpr bool operator!=(const Timestamp& o) const { return x_ != o.x_; }
  constexpr bool operator<=(const Timestamp& o) const { return x_ <= o.x_; }
  constexpr bool operator>=(const Timestamp& o) const { return x_ >= o.x_; }
  constexpr bool operator<(const Timestamp& o) const { return x_ < o.x_; }
  constexpr bool operator>(const Timestamp& o) const { return x_ > o.x_; }
  ///@}

  ///@{ @name Assignment
  constexpr Timestamp& operator=(const Timestamp&) = default;
  constexpr Timestamp& operator=(Timestamp&&) noexcept = default;
  ///@}

  ///@{ @name Add or subtract microseconds
  constexpr Timestamp operator++(int) const {
    Timestamp t(*this);
    ++t.x_;
    return t;
  }

  constexpr Timestamp operator--(int) const {
    Timestamp t(*this);
    --t.x_;
    return t;
  }

  constexpr Timestamp& operator++() {
    ++x_;
    return *this;
  }

  constexpr Timestamp& operator--() {
    --x_;
    return *this;
  }

  constexpr Timestamp& operator+=(value_type v) {
    x_ += v;
    return *this;
  }

  constexpr Timestamp& operator-=(value_type v) {
    x_ -= v;
    return *this;
  }

  constexpr Timestamp operator+(value_type v) const {
    Timestamp t(*this);
    t += v;
    return t;
  }

  constexpr Timestamp operator-(value_type v) const {
    Timestamp t(*this);
    t -= v;
    return t;
  }
  ///@}

  ///@{ @name Adds or subtract seconds
  constexpr Timestamp& add_seconds(double v) {
    x_ += value_type(v * 1000000.0);
    return *this;
  }

  constexpr Timestamp& add_micros(int64_t v) {
    x_ += v;
    return *this;
  }

  constexpr Timestamp& add_millis(int64_t v) { return add_micros(v * 1000); }
  constexpr Timestamp& add_seconds(int64_t v) { return add_micros(v * 1000000); }
  constexpr Timestamp& add_minutes(int64_t v) { return add_seconds(v * 60); }
  constexpr Timestamp& add_hours(int64_t v) { return add_seconds(v * 3600); }
  constexpr Timestamp& add_days(int64_t v) { return add_seconds(v * 86400); }

  constexpr Timestamp& add_months(int v) {
    int y, m, d, hour, min, sec, micros;
    unpack(y, m, d, hour, min, sec, micros);
    const int m0 = m - 1 + v; // [0..11]
    const int dy = m0 / 12;
    const int m1 = ((m0 < 0) ? (12 - (-m0 % 12)) : (m0 % 12)) + 1;
    const int dim = days_in_month(y + dy, m1);
    if (dim < d)
      d = dim;
    assert(is_valid_date(y + dy, m1, d));
    *this = Timestamp{y + dy, m1, d, hour, min, sec, micros};
    return *this;
  }

  constexpr Timestamp& add_years(int v) {
    int y, m, d, hour, min, sec, micros;
    unpack(y, m, d, hour, min, sec, micros);
    *this = Timestamp{y + v, m, d, hour, min, sec, micros};
    return *this;
  }

  ///@}

  ///@{ @name Distance metrics
  constexpr value_type micros_to(const Timestamp& v) const { return v.x_ - x_; }
  constexpr double seconds_to(const Timestamp& v) const {
    value_type diff = micros_to(v);
    value_type s_part = diff / M;
    value_type m_part = diff % M;
    return double(s_part) + double(m_part) * 1e-6;
  }
  ///@}

  ///@{ @name Getters
  constexpr int year() const {
    int y, m, d, hour, min, sec, micros;
    unpack(y, m, d, hour, min, sec, micros);
    return y;
  }

  constexpr int month() const {
    int y, m, d, hour, min, sec, micros;
    unpack(y, m, d, hour, min, sec, micros);
    return m;
  }

  constexpr int day() const {
    int y, m, d, hour, min, sec, micros;
    unpack(y, m, d, hour, min, sec, micros);
    return d;
  }

  constexpr int hour() const {
    int y, m, d, hour, min, sec, micros;
    unpack(y, m, d, hour, min, sec, micros);
    return hour;
  }

  constexpr int minute() const {
    int y, m, d, hour, min, sec, micros;
    unpack(y, m, d, hour, min, sec, micros);
    return min;
  }

  constexpr int second() const {
    int y, m, d, hour, min, sec, micros;
    unpack(y, m, d, hour, min, sec, micros);
    return sec;
  }

  constexpr int micros() const {
    int m = int(x_ % M);
    return (m < 0) ? (m + int(M)) : m;
  }

  constexpr void unpack(int& y, int& m, int& d, int& hour, int& min, int& sec, int& micros) const {
    std::tm tm = to_tm();
    y = tm.tm_year + 1900;
    m = tm.tm_mon + 1;
    d = tm.tm_mday;
    hour = tm.tm_hour;
    min = tm.tm_min;
    sec = tm.tm_sec;
    micros = int(this->micros());
  }

  constexpr std::tm to_tm() const {
    std::tm tm;
    seconds_to_tm(seconds_from_epoch(), &tm);
    return tm;
  }

  constexpr int64_t seconds_from_epoch() const {
    auto div = int64_t(x_ / M);
    if (div > 0)
      return div;
    auto rem = x_ % M;
    return (rem < 0) ? (div - 1) : div;
  }

  constexpr value_type value() const { return x_; }

  constexpr std::chrono::system_clock::time_point time_point() const {
    return std::chrono::system_clock::time_point{std::chrono::microseconds{x_}};
  }
  ///@}

  ///@{ @name Setters
  constexpr void set_micros(int micros) {
    assert(micros >= 0 && micros < M);
    x_ = value_type(seconds_from_epoch()) * M + micros;
  }

  constexpr void set_seconds_to_epoch(int64_t s) { x_ = value_type(s) * M + value_type(micros()); }

  constexpr void set(int64_t s, int micros) {
    set_seconds_to_epoch(s);
    set_micros(micros);
  }

  constexpr void set(int y, int m, int d, int hour = 0, int min = 0, int sec = 0, int micros = 0) {
    if (!is_valid_date(y, m, d))
      throw std::invalid_argument(fmt::format("invalid date: {:04d}-{:02d}-{:02d}", y, m, d));
    if (!is_valid_time(hour, min, sec))
      throw std::invalid_argument(
          fmt::format("invalid time: {:02d}:{:02d}:{:02d}", hour, min, sec));
    if (micros < 0 or micros > 999999)
      throw std::invalid_argument(fmt::format("invalid microseconds: {}", micros));

    const auto epoch = datetime_to_seconds(y, m, d, hour, min, sec);
    set(epoch, micros);
  }
  ///@}

  ///@{
  std::string to_string() const {
    std::tm tm = to_tm();
    return fmt::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}.{:06d}", tm.tm_year + 1900,
                       tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, micros());
  }

  std::string to_short_string() const {
    std::tm tm = to_tm();
    return fmt::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}", tm.tm_year + 1900,
                       tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  }

  ///@}

  /// @brief String-shim function.
  friend inline std::string str(const Timestamp& x) { return x.to_string(); }

  ///@brief The distance (in seconds) between two Timestamps.
  ///       I.e., `(b - a)` in seconds.
  friend constexpr double distance(const Timestamp& a, const Timestamp& b) {
    return a.seconds_to(b);
  }

  /// @brief Add seconds to a timestamp, returning a new instance by value.
  friend constexpr Timestamp add_seconds(const Timestamp& t, double s) {
    Timestamp r(t);
    r.add_seconds(s);
    return r;
  }
};

// ---------------------------------------------------------------- is leap year
/**@brief Test if a year is a leap year.
 * @ingroup date-time
 * @relates Timestamp
 * @return TRUE iff the passed year is a (gregorian) leap year.
 */
constexpr bool is_leap_year(int y) {
  if (y < 0)
    return is_leap_year(-y);
  return (y % 4 == 0) and ((y % 100 != 0) or (y % 400 == 0));
}

// --------------------------------------------------------------- is valid date
/**@brief Test if the passed year/month/day combination is a valid date.
 * @ingroup date-time
 * @relates Timestamp
 */
constexpr bool is_valid_date(int y, int m, int d) {
  if (m < 1 or m > 12)
    return false;
  if (d < 1 or d > 31)
    return false;
  if (d > 30 and (m == 4 or m == 6 or m == 9 or m == 11))
    return false;
  if (m == 2) {
    if (d > 29)
      return false;
    if (d == 29 and not is_leap_year(y))
      return false;
  }
  return true;
}

// --------------------------------------------------------------- is valid time
/**@brief Test if the passed hour/minute/second combination is a valid time
 * @ingroup date-time
 * @relates Timestamp
 */
constexpr bool is_valid_time(int h, int m, int s) {
  return (h >= 0 and h < 24) and (m >= 0 and m < 60) and (s >= 0 and s < 60);
}

// --------------------------------------------------------------- days in month
/**@brief The number of days in a month
 * @ingroup date-time
 * @relates Timestamp
 */
constexpr int days_in_month(int y, int m) // [1..12]
{
  assert(m >= 1 && m <= 12);
  if (m == 4 or m == 6 or m == 9 or m == 11)
    return 30;
  if (m == 2)
    return is_leap_year(y) ? 29 : 28;
  return 31;
}

// ----------------------------------------------------------------- day of week
/**@brief The day of the week (0->Sunday, 6->Saturday) for given year/month/day.
 * @ingroup date-time
 * @relates Timestamp
 */
constexpr int day_of_week(int y, int m, int d) {
  assert(is_valid_date(y, m, d));
  m -= 2;
  if (m < 1) {
    m += 12; // March is 1, Feb is 12
    y -= 1;
  }

  int D = y % 100;
  int C = y / 100;
  int f = d + (13 * m - 1) / 5 + D + D / 4 + C / 4 - 2 * C;
  int w = f % 7;
  if (w < 0)
    w += 7;
  return w;
}

// ----------------------------------------------------------------- day of year
/**@brief The day of the year, for example, Jan/1 is the 0th day of the year.
 * @ingroup date-time
 * @relates Timestamp
 */
constexpr int day_of_year(int y, int m, int d) {
  assert(is_valid_date(y, m, d));
  assert(m >= 1 and m <= 12);
  int ret = (d - 1) + (m > 2 && is_leap_year(y) ? 1 : 0);
  constexpr std::array<int, 12> L{{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334}};
  if (m >= 1 && m <= 12)
    ret += L[size_t(m - 1)];
  return ret;
}

// --------------------------------------------------------------- Seconds to TM
/**@brief Converts "seconds from epoch" to a `std::tm` struct
 * @ingroup date-time
 * @relates Timestamp
 * @author The developers of MUSL libc, MIT license.
 */
constexpr bool seconds_to_tm(int64_t t, std::tm* tm) {
  // 2000-03-01 (mod 400 year, immediately after feb29
  constexpr int64_t LEAPOCH = (946684800LL + 86400 * (31 + 29));
  constexpr int64_t DAYS_PER_400Y = (365 * 400 + 97);
  constexpr int64_t DAYS_PER_100Y = (365 * 100 + 24);
  constexpr int64_t DAYS_PER_4Y = (365 * 4 + 1);

  long long days, secs;
  int remdays, remsecs, remyears;
  int qc_cycles, c_cycles, q_cycles;
  int years, months;
  int wday, yday, leap;
  constexpr int8_t days_in_month[] = {31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29};

  // Reject time_t values whose year would overflow int
  if (t < INT_MIN * 31622400LL || t > INT_MAX * 31622400LL)
    return false;

  secs = t - LEAPOCH;
  days = secs / 86400;
  remsecs = secs % 86400;
  if (remsecs < 0) {
    remsecs += 86400;
    days--;
  }

  wday = (3 + days) % 7;
  if (wday < 0)
    wday += 7;

  qc_cycles = int(days / DAYS_PER_400Y);
  remdays = days % DAYS_PER_400Y;
  if (remdays < 0) {
    remdays += DAYS_PER_400Y;
    qc_cycles--;
  }

  c_cycles = remdays / DAYS_PER_100Y;
  if (c_cycles == 4)
    c_cycles--;
  remdays -= c_cycles * DAYS_PER_100Y;

  q_cycles = remdays / DAYS_PER_4Y;
  if (q_cycles == 25)
    q_cycles--;
  remdays -= q_cycles * DAYS_PER_4Y;

  remyears = remdays / 365;
  if (remyears == 4)
    remyears--;
  remdays -= remyears * 365;

  leap = !remyears and (q_cycles or !c_cycles);
  yday = remdays + 31 + 28 + leap;
  if (yday >= 365 + leap)
    yday -= 365 + leap;

  years = remyears + 4 * q_cycles + 100 * c_cycles + 400 * qc_cycles;

  for (months = 0; days_in_month[months] <= remdays; months++)
    remdays -= days_in_month[months];

  if (years + 100 > INT_MAX || years + 100 < INT_MIN)
    return false;

  tm->tm_year = years + 100;
  tm->tm_mon = months + 2;
  if (tm->tm_mon >= 12) {
    tm->tm_mon -= 12;
    tm->tm_year++;
  }
  tm->tm_mday = remdays + 1;
  tm->tm_wday = wday;
  tm->tm_yday = yday;

  tm->tm_hour = remsecs / 3600;
  tm->tm_min = remsecs / 60 % 60;
  tm->tm_sec = remsecs % 60;

  return true;
}

// ---------------------------------------------------------------- year to secs
/**@brief Converts a year to "seconds from epoch", optionally storing leap-year.
 * @ingroup date-time
 * @relates Timestamp
 * @author The developers of MUSL libc, MIT license.
 */
constexpr int64_t year_to_secs(int64_t year, int* is_leap) {
  assert(is_leap != nullptr);

  if (uint64_t(year) - 2ULL <= 136) {
    auto y = year;
    auto leaps = (y - 68) >> 2;
    if (!((y - 68) & 3)) {
      leaps--;
      if (is_leap)
        *is_leap = 1;
    } else if (is_leap) {
      *is_leap = 0;
    }
    return 31536000 * (y - 70) + 86400 * leaps;
  }

  int cycles, centuries, leaps, rem;

  cycles = int((year - 100) / 400);
  rem = int((year - 100) % 400);
  if (rem < 0) {
    cycles--;
    rem += 400;
  }
  if (!rem) {
    *is_leap = 1;
    centuries = 0;
    leaps = 0;
  } else {
    if (rem >= 200) {
      if (rem >= 300) {
        centuries = 3;
        rem -= 300;
      } else {
        centuries = 2;
        rem -= 200;
      }
    } else {
      if (rem >= 100) {
        centuries = 1;
        rem -= 100;
      } else {
        centuries = 0;
      }
    }
    if (!rem) {
      *is_leap = 0;
      leaps = 0;
    } else {
      leaps = int(unsigned(rem) / 4U);
      rem %= 4U;
      *is_leap = !rem;
    }
  }

  leaps += 97 * cycles + 24 * centuries - *is_leap;

  return (year - 100) * 31536000LL + leaps * 86400LL + 946684800 + 86400;
}

// --------------------------------------------------------- datetime to seconds
/**@brief Converts a year/month/day/hour/min/second to "seconds from epoch".
 * @ingroup date-time
 * @relates Timestamp
 * @author The developers of MUSL libc, MIT license.
 */
constexpr int64_t datetime_to_seconds(int y, int m, int d, int H, int M, int S) {
  assert(m >= 1 && m <= 12);
  assert(d >= 1 && d <= 31);
  assert(H >= 0 && H <= 23);
  assert(M >= 0 && M <= 59);
  assert(S >= 0 && S <= 59);
  int is_leap;
  int year = y - 1900;
  int month = m - 1;
  int64_t t = year_to_secs(year, &is_leap);
  t += day_of_year(y, m, d) * 86400LL;
  t += 3600LL * H;
  t += 60LL * M;
  t += S;
  return t;
}

} // namespace niggly
