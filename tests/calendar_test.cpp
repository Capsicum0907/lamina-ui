#include <doctest/doctest.h>

#include "lamina/calendar/date.hpp"

using namespace lamina::calendar;

TEST_CASE("leap years follow the Gregorian rule, centuries included") {
    CHECK(is_leap_year(2024));
    CHECK_FALSE(is_leap_year(2023));
    CHECK_FALSE(is_leap_year(1900));
    CHECK_FALSE(is_leap_year(2100));
    CHECK(is_leap_year(2000));
    CHECK(is_leap_year(1600));
    CHECK(is_leap_year(0));
    CHECK(is_leap_year(-4));
    CHECK_FALSE(is_leap_year(-100));
    CHECK(is_leap_year(-400));
    CHECK(days_in_month(2024, 2) == 29);
    CHECK(days_in_month(2023, 2) == 28);
    CHECK(days_in_month(1900, 2) == 28);
    CHECK(days_in_month(2000, 2) == 29);
    CHECK(days_in_month(2026, 4) == 30);
    CHECK(days_in_month(2026, 12) == 31);
    CHECK(days_in_month(2026, 13) == 0);
    CHECK(days_in_year(2000) == 366);
    CHECK(days_in_year(2100) == 365);
}

TEST_CASE("day numbers count from 1970-01-01 and round-trip over many centuries") {
    CHECK(days_from_civil({1970, 1, 1}) == 0);
    CHECK(days_from_civil({1969, 12, 31}) == -1);
    CHECK(days_from_civil({2000, 3, 1}) == 11017);
    CHECK(days_from_civil({2026, 11, 21}) == 20778);
    CHECK(civil_from_days(20778) == Date{2026, 11, 21});
    CHECK(days_from_civil({0, 3, 1}) == -719468);
    CHECK(civil_from_days(-719469) == Date{0, 2, 29});
    CHECK(days_between({1582, 10, 4}, {1582, 10, 15}) == 11);
    for (std::int64_t d = -800000; d <= 800000; d += 997) {
        const Date date = civil_from_days(d);
        CHECK(is_valid(date));
        CHECK(days_from_civil(date) == d);
    }
    Date prev = civil_from_days(-1000);
    for (std::int64_t d = -999; d <= 1000; ++d) {
        const Date next = civil_from_days(d);
        CHECK(next > prev);
        prev = next;
    }
}

TEST_CASE("weekdays are proleptic Gregorian") {
    CHECK(weekday({1970, 1, 1}) == Weekday::thursday);
    CHECK(weekday({2026, 11, 21}) == Weekday::saturday);
    CHECK(weekday({2000, 1, 1}) == Weekday::saturday);
    CHECK(weekday({2000, 2, 29}) == Weekday::tuesday);
    CHECK(weekday({1900, 3, 1}) == Weekday::thursday);
    CHECK(weekday({1582, 10, 15}) == Weekday::friday);
    CHECK(weekday({1, 1, 1}) == Weekday::monday);
    CHECK(weekday({0, 12, 31}) == Weekday::sunday);
    CHECK(weekday({-1, 1, 1}) == Weekday::friday);
    CHECK(weekday({2100, 3, 1}) == Weekday::monday);
}

TEST_CASE("adding months clamps the day to the end of the shorter month") {
    CHECK(add_months(Date{2024, 1, 31}, 1) == Date{2024, 2, 29});
    CHECK(add_months(Date{2023, 1, 31}, 1) == Date{2023, 2, 28});
    CHECK(add_months(Date{2024, 3, 31}, -1) == Date{2024, 2, 29});
    CHECK(add_months(Date{2026, 11, 21}, 2) == Date{2027, 1, 21});
    CHECK(add_months(Date{2026, 1, 15}, -1) == Date{2025, 12, 15});
    CHECK(add_months(YearMonth{1, 1}, -1) == YearMonth{0, 12});
    CHECK(add_months(YearMonth{0, 1}, -13) == YearMonth{-2, 12});
    CHECK(add_months(YearMonth{-1, 12}, 1) == YearMonth{0, 1});
    CHECK(months_between({2026, 11}, {2027, 2}) == 3);
    CHECK(months_between({2027, 2}, {2026, 11}) == -3);
    CHECK(add_days(Date{2024, 2, 28}, 1) == Date{2024, 2, 29});
    CHECK(add_days(Date{2023, 2, 28}, 1) == Date{2023, 3, 1});
    CHECK(add_days(Date{2026, 12, 31}, 1) == Date{2027, 1, 1});
    CHECK(add_days(Date{2027, 1, 1}, -1) == Date{2026, 12, 31});
}

TEST_CASE("a month's grid starts on the locale's first day of the week") {
    CHECK(leading_days({2026, 11}, Weekday::sunday) == 0);
    CHECK(leading_days({2026, 11}, Weekday::monday) == 6);
    CHECK(weeks_in_month({2026, 11}, Weekday::sunday) == 5);
    CHECK(weeks_in_month({2026, 11}, Weekday::monday) == 6);
    CHECK(weeks_in_month({2015, 2}, Weekday::sunday) == 4);
    CHECK(weeks_in_month({2026, 2}, Weekday::sunday) == 4);
    CHECK(weeks_in_month({2026, 2}, Weekday::monday) == 5);
    CHECK(date_in_cell({2026, 11}, Weekday::monday, 5) == std::nullopt);
    CHECK(date_in_cell({2026, 11}, Weekday::monday, 6) == Date{2026, 11, 1});
    CHECK(date_in_cell({2026, 11}, Weekday::monday, 35) == Date{2026, 11, 30});
    CHECK(date_in_cell({2026, 11}, Weekday::monday, 36) == std::nullopt);
    CHECK(cell_of({2026, 11, 21}, Weekday::sunday) == 20);
    CHECK(cell_of({2026, 11, 21}, Weekday::monday) == 26);
    CHECK(column_of(Weekday::sunday, Weekday::sunday) == 0);
    CHECK(column_of(Weekday::saturday, Weekday::sunday) == 6);
    CHECK(column_of(Weekday::sunday, Weekday::monday) == 6);
    CHECK(column_of(Weekday::monday, Weekday::saturday) == 2);
}

TEST_CASE("ranges include both ends") {
    const DateRange r{{2026, 11, 3}, {2026, 11, 9}};
    CHECK(r.contains({2026, 11, 3}));
    CHECK(r.contains({2026, 11, 9}));
    CHECK_FALSE(r.contains({2026, 11, 10}));
    const YearRange y{1900, 2100};
    CHECK(y.count() == 201);
    CHECK(y.contains(2100));
    CHECK_FALSE(y.contains(1899));
}
