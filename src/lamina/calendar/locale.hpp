#pragma once

#include <string>

#include "lamina/calendar/date.hpp"
#include "lamina/calendar/format.hpp"

namespace lamina::calendar {

struct PickerStrings {
    std::string date_picker_title;
    std::string date_picker_headline;
    std::string date_input_title;
    std::string date_input_headline;
    std::string date_input_label;
    std::string date_picker_headline_description;
    std::string date_input_headline_description;
    std::string no_selection_description;
    std::string today_description;
    std::string switch_to_year_selection;
    std::string switch_to_day_selection;
    std::string next_month;
    std::string previous_month;
    std::string navigate_to_year;
    std::string year_picker_pane_title;
    std::string switch_to_calendar_mode;
    std::string switch_to_input_mode;
    std::string invalid_for_pattern;
    std::string invalid_year_range;
    std::string invalid_not_allowed;
    std::string range_picker_title;
    std::string range_input_title;
    std::string range_start_headline;
    std::string range_end_headline;
    std::string labelled_value;
    std::string day_in_range;
    std::string invalid_range_input;
    std::string time_picker_title;
    std::string time_input_title;
    std::string am;
    std::string pm;
    std::string period_toggle_description;
    std::string hour_selection;
    std::string minute_selection;
    std::string hour_suffix;
    std::string hour_24h_suffix;
    std::string minute_suffix;
    std::string hour;
    std::string minute;
    std::string hour_text_field;
    std::string minute_text_field;
    std::string switch_to_text_input;
    std::string switch_to_clock_mode;
    std::string hour_error;
    std::string hour_error_24h;
    std::string minute_error;
    std::string confirm;
    std::string dismiss;
    std::string range_delimiter;
    std::string list_separator;
};

struct CalendarLocale {
    std::string tag;
    Weekday first_day_of_week;
    CalendarNames names;
    std::string year_month_pattern;
    std::string short_date_pattern;
    std::string long_date_pattern;
    InputPattern input;
    bool uses_24_hour_clock;
    std::string time_12_pattern;
    std::string time_24_pattern;
    PickerStrings strings;
};

const CalendarLocale& english_us();
const CalendarLocale& english_gb();
const CalendarLocale& japanese();

std::string input_placeholder(const InputPattern& pattern);

}
