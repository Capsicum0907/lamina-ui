#pragma once

#include <compare>
#include <cstdint>
#include <optional>

namespace lamina::calendar {

enum class Weekday { monday, tuesday, wednesday, thursday, friday, saturday, sunday };

inline constexpr int days_per_week = 7;
inline constexpr int months_per_year = 12;

struct Date {
    int year = 1970;
    int month = 1;
    int day = 1;

    friend constexpr auto operator<=>(const Date&, const Date&) = default;
};

struct YearMonth {
    int year = 1970;
    int month = 1;

    friend constexpr auto operator<=>(const YearMonth&, const YearMonth&) = default;
};

constexpr bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

constexpr int days_in_month(int year, int month) {
    constexpr int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > months_per_year) {
        return 0;
    }
    return month == 2 && is_leap_year(year) ? 29 : days[month - 1];
}

constexpr int days_in_year(int year) {
    return is_leap_year(year) ? 366 : 365;
}

constexpr bool is_valid(const Date& d) {
    return d.month >= 1 && d.month <= months_per_year && d.day >= 1 && d.day <= days_in_month(d.year, d.month);
}

constexpr std::int64_t days_from_civil(const Date& d) {
    const std::int64_t y = static_cast<std::int64_t>(d.year) - (d.month <= 2 ? 1 : 0);
    const std::int64_t era = (y >= 0 ? y : y - 399) / 400;
    const std::int64_t yoe = y - era * 400;
    const std::int64_t mp = (d.month + 9) % 12;
    const std::int64_t doy = (153 * mp + 2) / 5 + d.day - 1;
    const std::int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

constexpr Date civil_from_days(std::int64_t days) {
    const std::int64_t z = days + 719468;
    const std::int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const std::int64_t doe = z - era * 146097;
    const std::int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const std::int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const std::int64_t mp = (5 * doy + 2) / 153;
    const int day = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    const int month = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
    const int year = static_cast<int>(yoe + era * 400 + (month <= 2 ? 1 : 0));
    return {year, month, day};
}

constexpr Weekday weekday(const Date& d) {
    const std::int64_t days = days_from_civil(d);
    const std::int64_t from_monday = ((days + 3) % 7 + 7) % 7;
    return static_cast<Weekday>(from_monday);
}

constexpr int weekday_index(Weekday w) {
    return static_cast<int>(w);
}

constexpr Weekday weekday_from_index(int index) {
    return static_cast<Weekday>(((index % days_per_week) + days_per_week) % days_per_week);
}

constexpr int column_of(Weekday w, Weekday first_day_of_week) {
    return ((weekday_index(w) - weekday_index(first_day_of_week)) % days_per_week + days_per_week) % days_per_week;
}

constexpr Date add_days(const Date& d, std::int64_t days) {
    return civil_from_days(days_from_civil(d) + days);
}

constexpr std::int64_t days_between(const Date& from, const Date& to) {
    return days_from_civil(to) - days_from_civil(from);
}

constexpr YearMonth year_month(const Date& d) {
    return {d.year, d.month};
}

constexpr YearMonth add_months(const YearMonth& ym, int months) {
    const std::int64_t index = static_cast<std::int64_t>(ym.year) * months_per_year + (ym.month - 1) + months;
    const std::int64_t year = index >= 0 ? index / months_per_year : -((-index - 1) / months_per_year) - 1;
    return {static_cast<int>(year), static_cast<int>(index - year * months_per_year) + 1};
}

constexpr int months_between(const YearMonth& from, const YearMonth& to) {
    return (to.year - from.year) * months_per_year + (to.month - from.month);
}

constexpr Date add_months(const Date& d, int months) {
    const YearMonth ym = add_months(year_month(d), months);
    const int last = days_in_month(ym.year, ym.month);
    return {ym.year, ym.month, d.day < last ? d.day : last};
}

constexpr Date first_day(const YearMonth& ym) {
    return {ym.year, ym.month, 1};
}

constexpr Date last_day(const YearMonth& ym) {
    return {ym.year, ym.month, days_in_month(ym.year, ym.month)};
}

constexpr int leading_days(const YearMonth& ym, Weekday first_day_of_week) {
    return column_of(weekday(first_day(ym)), first_day_of_week);
}

constexpr int weeks_in_month(const YearMonth& ym, Weekday first_day_of_week) {
    const int cells = leading_days(ym, first_day_of_week) + days_in_month(ym.year, ym.month);
    return (cells + days_per_week - 1) / days_per_week;
}

constexpr std::optional<Date> date_in_cell(const YearMonth& ym, Weekday first_day_of_week, int cell) {
    const int day = cell - leading_days(ym, first_day_of_week) + 1;
    if (day < 1 || day > days_in_month(ym.year, ym.month)) {
        return std::nullopt;
    }
    return Date{ym.year, ym.month, day};
}

constexpr int cell_of(const Date& d, Weekday first_day_of_week) {
    return leading_days(year_month(d), first_day_of_week) + d.day - 1;
}

struct DateRange {
    Date start;
    Date end;

    constexpr bool contains(const Date& d) const { return d >= start && d <= end; }
};

struct YearRange {
    int first;
    int last;

    constexpr bool contains(int year) const { return year >= first && year <= last; }
    constexpr int count() const { return last - first + 1; }
};

}
