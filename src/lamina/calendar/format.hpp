#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include "lamina/calendar/date.hpp"

namespace lamina::calendar {

struct CalendarNames {
    std::array<std::string, months_per_year> months_long;
    std::array<std::string, months_per_year> months_short;
    std::array<std::string, days_per_week> weekdays_long;
    std::array<std::string, days_per_week> weekdays_short;
    std::array<std::string, days_per_week> weekdays_narrow;
    std::array<std::string, 2> periods;
};

std::string format_date(std::string_view pattern, const Date& date, const CalendarNames& names);
std::string format_time(std::string_view pattern, int hour, int minute, const CalendarNames& names);
std::string format_number(int value, int min_digits = 1);
std::string substitute(std::string_view text, std::string_view arg);
std::string substitute(std::string_view text, std::string_view first, std::string_view second);

struct InputPattern {
    std::string pattern;
    char32_t delimiter;
};

std::size_t input_pattern_digits(const InputPattern& pattern);
std::string apply_input_pattern(const InputPattern& pattern, std::string_view digits);
std::optional<Date> parse_input(const InputPattern& pattern, std::string_view digits);
std::string input_digits(const InputPattern& pattern, const Date& date);

}
