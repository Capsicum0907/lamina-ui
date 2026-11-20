#include "lamina/calendar/format.hpp"

namespace lamina::calendar {

namespace {

bool is_pattern_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

std::size_t run_length(std::string_view pattern, std::size_t i) {
    std::size_t n = 1;
    while (i + n < pattern.size() && pattern[i + n] == pattern[i]) {
        ++n;
    }
    return n;
}

template <typename Field>
std::string format_pattern(std::string_view pattern, Field field) {
    std::string out;
    std::size_t i = 0;
    while (i < pattern.size()) {
        const char c = pattern[i];
        if (c == '\'') {
            if (i + 1 < pattern.size() && pattern[i + 1] == '\'') {
                out.push_back('\'');
                i += 2;
                continue;
            }
            std::size_t j = i + 1;
            while (j < pattern.size()) {
                if (pattern[j] == '\'') {
                    if (j + 1 < pattern.size() && pattern[j + 1] == '\'') {
                        out.push_back('\'');
                        j += 2;
                        continue;
                    }
                    break;
                }
                out.push_back(pattern[j]);
                ++j;
            }
            i = j + 1;
            continue;
        }
        if (!is_pattern_letter(c)) {
            out.push_back(c);
            ++i;
            continue;
        }
        const std::size_t n = run_length(pattern, i);
        if (!field(out, c, n)) {
            out.append(pattern.substr(i, n));
        }
        i += n;
    }
    return out;
}

}

std::string format_number(int value, int min_digits) {
    const bool negative = value < 0;
    std::string digits = std::to_string(negative ? -static_cast<long long>(value) : static_cast<long long>(value));
    while (static_cast<int>(digits.size()) < min_digits) {
        digits.insert(digits.begin(), '0');
    }
    return negative ? "-" + digits : digits;
}

std::string format_date(std::string_view pattern, const Date& date, const CalendarNames& names) {
    return format_pattern(pattern, [&](std::string& out, char c, std::size_t n) {
        switch (c) {
        case 'y':
            if (n == 2) {
                out += format_number(((date.year % 100) + 100) % 100, 2);
            } else {
                out += format_number(date.year, static_cast<int>(n));
            }
            return true;
        case 'M':
        case 'L':
            if (n >= 4) {
                out += names.months_long[static_cast<std::size_t>(date.month - 1)];
            } else if (n == 3) {
                out += names.months_short[static_cast<std::size_t>(date.month - 1)];
            } else {
                out += format_number(date.month, static_cast<int>(n));
            }
            return true;
        case 'd':
            out += format_number(date.day, static_cast<int>(n));
            return true;
        case 'E': {
            const auto w = static_cast<std::size_t>(weekday_index(weekday(date)));
            out += n == 4 ? names.weekdays_long[w] : n >= 5 ? names.weekdays_narrow[w] : names.weekdays_short[w];
            return true;
        }
        default:
            return false;
        }
    });
}

std::string format_time(std::string_view pattern, int hour, int minute, const CalendarNames& names) {
    return format_pattern(pattern, [&](std::string& out, char c, std::size_t n) {
        switch (c) {
        case 'H':
            out += format_number(hour, static_cast<int>(n));
            return true;
        case 'h': {
            const int h = hour % 12 == 0 ? 12 : hour % 12;
            out += format_number(h, static_cast<int>(n));
            return true;
        }
        case 'm':
            out += format_number(minute, static_cast<int>(n));
            return true;
        case 'a':
            out += names.periods[hour < 12 ? 0 : 1];
            return true;
        default:
            return false;
        }
    });
}

std::string substitute(std::string_view text, std::string_view arg) {
    return substitute(text, arg, {});
}

std::string substitute(std::string_view text, std::string_view first, std::string_view second) {
    std::string out;
    std::size_t i = 0;
    while (i < text.size()) {
        if (text[i] == '{' && i + 2 < text.size() && text[i + 2] == '}' && (text[i + 1] == '0' || text[i + 1] == '1')) {
            out += text[i + 1] == '0' ? first : second;
            i += 3;
            continue;
        }
        out.push_back(text[i]);
        ++i;
    }
    return out;
}

std::size_t input_pattern_digits(const InputPattern& pattern) {
    std::size_t n = 0;
    for (char c : pattern.pattern) {
        n += is_pattern_letter(c) ? 1 : 0;
    }
    return n;
}

std::string apply_input_pattern(const InputPattern& pattern, std::string_view digits) {
    std::string out;
    std::size_t used = 0;
    for (char c : pattern.pattern) {
        if (!is_pattern_letter(c)) {
            out.push_back(c);
            continue;
        }
        if (used >= digits.size()) {
            break;
        }
        out.push_back(digits[used++]);
    }
    return out;
}

std::optional<Date> parse_input(const InputPattern& pattern, std::string_view digits) {
    if (digits.size() != input_pattern_digits(pattern)) {
        return std::nullopt;
    }
    int year = 0;
    int month = 0;
    int day = 0;
    std::size_t used = 0;
    for (char c : pattern.pattern) {
        if (!is_pattern_letter(c)) {
            continue;
        }
        const char d = digits[used++];
        if (d < '0' || d > '9') {
            return std::nullopt;
        }
        const int v = d - '0';
        if (c == 'y') {
            year = year * 10 + v;
        } else if (c == 'M') {
            month = month * 10 + v;
        } else {
            day = day * 10 + v;
        }
    }
    const Date date{year, month, day};
    if (!is_valid(date)) {
        return std::nullopt;
    }
    return date;
}

std::string input_digits(const InputPattern& pattern, const Date& date) {
    std::string out;
    std::size_t i = 0;
    const std::string_view p = pattern.pattern;
    while (i < p.size()) {
        if (!is_pattern_letter(p[i])) {
            ++i;
            continue;
        }
        const std::size_t n = run_length(p, i);
        const int v = p[i] == 'y' ? date.year : p[i] == 'M' ? date.month : date.day;
        std::string s = format_number(v, static_cast<int>(n));
        out += s.substr(s.size() > n ? s.size() - n : 0);
        i += n;
    }
    return out;
}

}
