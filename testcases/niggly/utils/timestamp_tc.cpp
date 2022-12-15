
#include "niggly/utils/timestamp.hpp"

#include <catch2/catch_all.hpp>

namespace niggly::tests {

// ------------------------------------------------------------------- TEST_CASE
//
CATCH_TEST_CASE("Timestamp", "[timestamp]") {
  CATCH_SECTION("is_leap_year") {
    CATCH_REQUIRE(is_leap_year(1900) == false);
    CATCH_REQUIRE(is_leap_year(2000) == true);
    CATCH_REQUIRE(is_leap_year(2300) == false);
    CATCH_REQUIRE(is_leap_year(2400) == true);
    CATCH_REQUIRE(is_leap_year(2004) == true);
    CATCH_REQUIRE(is_leap_year(2005) == false);
  }

  CATCH_SECTION("days_in_month") {
    CATCH_REQUIRE(days_in_month(2000, 1) == 31);
    CATCH_REQUIRE(days_in_month(2000, 2) == 29); // leap year
    CATCH_REQUIRE(days_in_month(2000, 3) == 31);
    CATCH_REQUIRE(days_in_month(2000, 4) == 30);
    CATCH_REQUIRE(days_in_month(2000, 5) == 31);
    CATCH_REQUIRE(days_in_month(2000, 6) == 30);
    CATCH_REQUIRE(days_in_month(2000, 7) == 31);
    CATCH_REQUIRE(days_in_month(2000, 8) == 31);
    CATCH_REQUIRE(days_in_month(2000, 9) == 30);
    CATCH_REQUIRE(days_in_month(2000, 10) == 31);
    CATCH_REQUIRE(days_in_month(2000, 11) == 30);
    CATCH_REQUIRE(days_in_month(2000, 12) == 31);
    CATCH_REQUIRE(days_in_month(2001, 2) == 28); // not a leap year
  }

  CATCH_SECTION("day_of_week") {
    CATCH_REQUIRE(day_of_week(2022, 5, 1) == 0);
    CATCH_REQUIRE(day_of_week(2022, 5, 2) == 1);
    CATCH_REQUIRE(day_of_week(2022, 1, 31) == 1);
  }

  CATCH_SECTION("day_of_year") {
    CATCH_REQUIRE(day_of_year(2022, 1, 1) == 0);
    CATCH_REQUIRE(day_of_year(2022, 2, 1) == 31);
    CATCH_REQUIRE(day_of_year(2022, 12, 31) == 364);
  }

  CATCH_SECTION("is_valid_date") {
    CATCH_REQUIRE(is_valid_date(1904, 1, 1) == true);
    CATCH_REQUIRE(is_valid_date(1904, 4, 30) == true);
    CATCH_REQUIRE(is_valid_date(1904, 4, 31) == false);
    CATCH_REQUIRE(is_valid_date(1904, 5, 31) == true);
    CATCH_REQUIRE(is_valid_date(2000, 2, 29) == true);
    CATCH_REQUIRE(is_valid_date(2000, 2, 30) == false);
    CATCH_REQUIRE(is_valid_date(2001, 2, 29) == false);
  }

  CATCH_SECTION("datetime_to_seconds") {
    // Timestamps from https://www.onlineconversion.com/unix_time.htm
    CATCH_REQUIRE(datetime_to_seconds(1970, 1, 1, 0, 0, 0) == 0);
    CATCH_REQUIRE(datetime_to_seconds(1970, 1, 1, 0, 0, 1) == 1);
    CATCH_REQUIRE(datetime_to_seconds(1970, 1, 2, 0, 0, 0) == 86400);
    CATCH_REQUIRE(datetime_to_seconds(1970, 2, 1, 0, 0, 0) == 2678400);
    CATCH_REQUIRE(datetime_to_seconds(2022, 3, 4, 23, 22, 21) == 1646436141);
    CATCH_REQUIRE(datetime_to_seconds(2021, 12, 4, 23, 22, 21) == 1638660141);
    CATCH_REQUIRE(datetime_to_seconds(1961, 12, 4, 23, 22, 21) == -254795859);
  }

  CATCH_SECTION("timetamp") {
    CATCH_REQUIRE(Timestamp{}.value() == 0);

    {
      const auto t = Timestamp{100, 101};
      CATCH_REQUIRE(t.value() == 100 * 1000000 + 101);
    }

    {
      const auto t = Timestamp{100, 0};
      CATCH_REQUIRE(t.value() == 100 * 1000000);
    }

    {
      const auto t = Timestamp{100, 999999};
      CATCH_REQUIRE(t.value() == 100 * 1000000 + 999999);

      const auto u = add_seconds(t, 1.0);
      CATCH_REQUIRE(u.value() == 101 * 1000000 + 999999);

      CATCH_REQUIRE(distance(t, u) == 1.0);
      CATCH_REQUIRE(distance(u, t) == -1.0);
    }

    {
      const auto t = Timestamp{1915, 4, 30, 9, 14, 22, 123456};
      CATCH_REQUIRE(t.to_string() == "1915-04-30T09:14:22.123456");
      CATCH_REQUIRE(str(t) == "1915-04-30T09:14:22.123456");
      CATCH_REQUIRE(t.to_short_string() == "1915-04-30 09:14:22");
      CATCH_REQUIRE(t.year() == 1915);
      CATCH_REQUIRE(t.month() == 4);
      CATCH_REQUIRE(t.day() == 30);
      CATCH_REQUIRE(t.hour() == 9);
      CATCH_REQUIRE(t.minute() == 14);
      CATCH_REQUIRE(t.second() == 22);
      CATCH_REQUIRE(t.micros() == 123456);
    }

    {
      const auto t = Timestamp::parse("1915-04-30T09:14:22.123456");
      CATCH_REQUIRE(t.year() == 1915);
      CATCH_REQUIRE(t.month() == 4);
      CATCH_REQUIRE(t.day() == 30);
      CATCH_REQUIRE(t.hour() == 9);
      CATCH_REQUIRE(t.minute() == 14);
      CATCH_REQUIRE(t.second() == 22);
      CATCH_REQUIRE(t.micros() == 123456);
    }

    {
      // length
      CATCH_REQUIRE_THROWS(Timestamp::parse(""));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-3"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("012345678901234567890123456"));

      // delimiters
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04:30"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915:04-30"));
      CATCH_REQUIRE_NOTHROW(Timestamp::parse("1915-04-30t"));
      CATCH_REQUIRE_NOTHROW(Timestamp::parse("1915-04-30T"));
      CATCH_REQUIRE_NOTHROW(Timestamp::parse("1915-04-30 "));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30z"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 09.14.22"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 09.14:22"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 09:14.22"));
      CATCH_REQUIRE_NOTHROW(Timestamp::parse("1915-04-30 09:14:22"));
      CATCH_REQUIRE_NOTHROW(Timestamp::parse("1915-04-30 09-14-22"));
      CATCH_REQUIRE_NOTHROW(Timestamp::parse("1915-04-30 09:14:22."));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 09:14:22 "));

      // Digits
      CATCH_REQUIRE_NOTHROW(Timestamp::parse("1915-04-30 09:14:22.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("x915-04-30 09:14:22.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1x15-04-30 09:14:22.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("19x5-04-30 09:14:22.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("191x-04-30 09:14:22.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-x4-30 09:14:22.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-0x-30 09:14:22.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-x0 09:14:22.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-3x 09:14:22.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 x9:14:22.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 0x:14:22.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 09:x4:22.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 09:1x:22.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 09:14:x2.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 09:14:2x.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 09:14:22.x23456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 09:14:22.1x3456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 09:14:22.12x456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 09:14:22.123x56"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 09:14:22.1234x6"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30 09:14:22.12345x"));

      // Invalid time
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-31T09:14:22.123456"));
      CATCH_REQUIRE_THROWS(Timestamp::parse("1915-04-30T24:14:22.123456"));
    }

    {
      CATCH_REQUIRE_NOTHROW(Timestamp::now());
      CATCH_REQUIRE(Timestamp::min().value() < 0);
      CATCH_REQUIRE(Timestamp::max().value() > 0);
    }

    {
      Timestamp t;
      CATCH_REQUIRE_NOTHROW(t.set(1915, 4, 30, 9, 14, 23, 123456));
      CATCH_REQUIRE_THROWS(t.set(1915, 0, 30, 9, 14, 23, 123456));
      CATCH_REQUIRE_THROWS(t.set(1915, 13, 30, 9, 14, 23, 123456));
      CATCH_REQUIRE_THROWS(t.set(1915, 1, 32, 9, 14, 23, 123456));
      CATCH_REQUIRE_THROWS(t.set(1915, 1, 30, -1, 14, 23, 123456));
      CATCH_REQUIRE_THROWS(t.set(1915, 1, 30, 24, 14, 23, 123456));
      CATCH_REQUIRE_THROWS(t.set(1915, 1, 30, 23, -1, 23, 123456));
      CATCH_REQUIRE_THROWS(t.set(1915, 1, 30, 23, 60, 23, 123456));
      CATCH_REQUIRE_THROWS(t.set(1915, 1, 30, 23, 59, -1, 123456));
      CATCH_REQUIRE_THROWS(t.set(1915, 1, 30, 23, 59, 60, 123456));
      CATCH_REQUIRE_THROWS(t.set(1915, 1, 30, 23, 59, 59, -1));
      CATCH_REQUIRE_THROWS(t.set(1915, 1, 30, 23, 59, 59, 1000000));
    }
  }
}

} // namespace niggly::tests
