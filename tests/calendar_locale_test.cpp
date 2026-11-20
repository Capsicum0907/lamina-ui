#include <doctest/doctest.h>

#include "lamina/calendar/format.hpp"
#include "lamina/calendar/locale.hpp"

using namespace lamina::calendar;

TEST_CASE("dates format with the locale's CLDR patterns") {
    const Date d{2026, 11, 21};
    const CalendarLocale& us = english_us();
    CHECK(format_date(us.short_date_pattern, d, us.names) == "Nov 21, 2026");
    CHECK(format_date(us.long_date_pattern, d, us.names) == "Saturday, November 21, 2026");
    CHECK(format_date(us.year_month_pattern, d, us.names) == "November 2026");
    const CalendarLocale& gb = english_gb();
    CHECK(format_date(gb.short_date_pattern, d, gb.names) == "21 Nov 2026");
    CHECK(format_date(gb.long_date_pattern, d, gb.names) == "Saturday 21 November 2026");
    const CalendarLocale& ja = japanese();
    CHECK(format_date(ja.short_date_pattern, d, ja.names) == "2026年11月21日");
    CHECK(format_date(ja.long_date_pattern, d, ja.names) == "2026年11月21日土曜日");
    CHECK(format_date(ja.year_month_pattern, d, ja.names) == "2026年11月");
}

TEST_CASE("patterns pad, quote and leave unknown letters alone") {
    const CalendarNames& n = english_us().names;
    CHECK(format_date("yyyy-MM-dd", {7, 3, 4}, n) == "0007-03-04");
    CHECK(format_date("yy", {2026, 1, 1}, n) == "26");
    CHECK(format_date("d 'of' MMMM", {2026, 1, 1}, n) == "1 of January");
    CHECK(format_date("EEEEE", {2026, 11, 22}, n) == "S");
    CHECK(format_date("''", {2026, 1, 1}, n) == "'");
    CHECK(format_date("Q", {2026, 1, 1}, n) == "Q");
    CHECK(format_time("h:mm a", 0, 5, n) == "12:05 AM");
    CHECK(format_time("h:mm a", 13, 30, n) == "1:30 PM");
    CHECK(format_time("HH:mm", 9, 7, n) == "09:07");
    CHECK(format_number(-3, 2) == "-03");
}

TEST_CASE("templates take positional arguments") {
    CHECK(substitute("Navigate to year {0}", "2026") == "Navigate to year 2026");
    CHECK(substitute(english_us().strings.invalid_year_range, "1900", "2100") == "Date out of expected year range 1900 - 2100");
    CHECK(substitute(japanese().strings.invalid_year_range, "1900", "2100") == "想定される年の範囲（1900～2100）から日付が外れています");
    CHECK(substitute("{2} {0}", "a") == "{2} a");
}

TEST_CASE("input patterns follow the locale and insert delimiters once a field is complete") {
    const InputPattern& us = english_us().input;
    const InputPattern& ja = japanese().input;
    const InputPattern& gb = english_gb().input;
    CHECK(input_placeholder(us) == "MM/DD/YYYY");
    CHECK(input_placeholder(ja) == "YYYY/MM/DD");
    CHECK(input_placeholder(gb) == "DD/MM/YYYY");
    CHECK(input_pattern_digits(us) == 8);
    CHECK(apply_input_pattern(us, "") == "");
    CHECK(apply_input_pattern(us, "1") == "1");
    CHECK(apply_input_pattern(us, "11") == "11/");
    CHECK(apply_input_pattern(us, "1121") == "11/21/");
    CHECK(apply_input_pattern(us, "11212026") == "11/21/2026");
    CHECK(apply_input_pattern(ja, "20261") == "2026/1");
    CHECK(parse_input(us, "11212026") == Date{2026, 11, 21});
    CHECK(parse_input(ja, "20261121") == Date{2026, 11, 21});
    CHECK(parse_input(gb, "21112026") == Date{2026, 11, 21});
    CHECK(parse_input(us, "02292023") == std::nullopt);
    CHECK(parse_input(us, "02292024") == Date{2024, 2, 29});
    CHECK(parse_input(us, "13012024") == std::nullopt);
    CHECK(parse_input(us, "1121202") == std::nullopt);
    CHECK(input_digits(us, {2026, 3, 7}) == "03072026");
    CHECK(input_digits(ja, {2026, 3, 7}) == "20260307");
}

TEST_CASE("the locales differ in the first day of the week and the clock") {
    CHECK(english_us().first_day_of_week == Weekday::sunday);
    CHECK(japanese().first_day_of_week == Weekday::sunday);
    CHECK(english_gb().first_day_of_week == Weekday::monday);
    CHECK_FALSE(english_us().uses_24_hour_clock);
    CHECK(japanese().uses_24_hour_clock);
    CHECK(japanese().names.weekdays_narrow[6] == "日");
    CHECK(english_us().strings.am == "AM");
    CHECK(japanese().strings.am == "AM");
}
