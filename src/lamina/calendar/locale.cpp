#include "lamina/calendar/locale.hpp"

namespace lamina::calendar {

namespace {

const CalendarNames english_names{
    {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"},
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"},
    {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"},
    {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"},
    {"M", "T", "W", "T", "F", "S", "S"},
    {"AM", "PM"},
};

const PickerStrings english_strings{
    .date_picker_title = "Select date",
    .date_picker_headline = "Selected date",
    .date_input_title = "Select date",
    .date_input_headline = "Entered date",
    .date_input_label = "Date",
    .date_picker_headline_description = "Current selection: {0}",
    .date_input_headline_description = "Entered date: {0}",
    .no_selection_description = "None",
    .today_description = "Today",
    .switch_to_year_selection = "Activate to change the year",
    .switch_to_day_selection = "Activate to change month and date",
    .next_month = "Change to next month",
    .previous_month = "Change to previous month",
    .navigate_to_year = "Navigate to year {0}",
    .year_picker_pane_title = "Year picker visible",
    .switch_to_calendar_mode = "Switch to calendar input mode",
    .switch_to_input_mode = "Switch to text input mode",
    .invalid_for_pattern = "Date does not match expected pattern: {0}",
    .invalid_year_range = "Date out of expected year range {0} - {1}",
    .invalid_not_allowed = "Date not allowed: {0}",
    .range_picker_title = "Select dates",
    .range_input_title = "Enter dates",
    .range_start_headline = "Start date",
    .range_end_headline = "End date",
    .labelled_value = "{0}: {1}",
    .day_in_range = "In range",
    .invalid_range_input = "Invalid date range input",
    .time_picker_title = "Select Time",
    .time_input_title = "Enter Time",
    .am = "AM",
    .pm = "PM",
    .period_toggle_description = "Select AM or PM",
    .hour_selection = "Select hour",
    .minute_selection = "Select minutes",
    .hour_suffix = "{0} o'clock",
    .hour_24h_suffix = "{0} hours",
    .minute_suffix = "{0} minutes",
    .hour = "Hour",
    .minute = "Minute",
    .hour_text_field = "for hour",
    .minute_text_field = "for minutes",
    .switch_to_text_input = "Switch to text input mode",
    .switch_to_clock_mode = "Switch to clock mode",
    .hour_error = "Hour must be 1–12",
    .hour_error_24h = "Hour must be 0–23",
    .minute_error = "Minute must be 0–59",
    .confirm = "OK",
    .dismiss = "Cancel",
    .range_delimiter = "-",
    .list_separator = ", ",
};

const CalendarNames japanese_names{
    {"1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月"},
    {"1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月"},
    {"月曜日", "火曜日", "水曜日", "木曜日", "金曜日", "土曜日",
     "日曜日"},
    {"月", "火", "水", "木", "金", "土", "日"},
    {"月", "火", "水", "木", "金", "土", "日"},
    {"AM", "PM"},
};

const PickerStrings japanese_strings{
    .date_picker_title = "日付の選択",
    .date_picker_headline = "選択した日付",
    .date_input_title = "日付の選択",
    .date_input_headline = "入力された日付",
    .date_input_label = "日付",
    .date_picker_headline_description = "現在の選択: {0}",
    .date_input_headline_description = "入力された日付: {0}",
    .no_selection_description = "なし",
    .today_description = "今日",
    .switch_to_year_selection = "有効にすると年を変更できます",
    .switch_to_day_selection = "有効にすると月と日を変更できます",
    .next_month = "翌月に変更",
    .previous_month = "前月に変更",
    .navigate_to_year = "年に移動 {0}",
    .year_picker_pane_title = "年の選択ツールの表示",
    .switch_to_calendar_mode = "カレンダー入力モードに切り替え",
    .switch_to_input_mode = "テキスト入力モードに切り替え",
    .invalid_for_pattern = "想定パターンと一致しない日付: {0}",
    .invalid_year_range = "想定される年の範囲（{0}～{1}）から日付が外れています",
    .invalid_not_allowed = "許可されない日付: {0}",
    .range_picker_title = "日付の選択",
    .range_input_title = "日付の入力",
    .range_start_headline = "開始日",
    .range_end_headline = "終了日",
    .labelled_value = "{0}: {1}",
    .day_in_range = "範囲内",
    .invalid_range_input = "入力された期間は無効です",
    .time_picker_title = "時刻の選択",
    .time_input_title = "時刻の入力",
    .am = "AM",
    .pm = "PM",
    .period_toggle_description = "午前または午後を選択",
    .hour_selection = "時刻を選択",
    .minute_selection = "分を選択",
    .hour_suffix = "{0} 時",
    .hour_24h_suffix = "{0} 時間",
    .minute_suffix = "{0} 分",
    .hour = "時間",
    .minute = "分",
    .hour_text_field = "（時間単位）",
    .minute_text_field = "（分単位）",
    .switch_to_text_input = "テキスト入力モードに切り替える",
    .switch_to_clock_mode = "時計モードに切り替える",
    .hour_error = "時間は 1～12 の範囲で指定してください",
    .hour_error_24h = "時間は 0～23 の範囲で指定してください",
    .minute_error = "分は 0～59 の範囲で指定してください",
    .confirm = "OK",
    .dismiss = "キャンセル",
    .range_delimiter = "-",
    .list_separator = "、",
};

}

const CalendarLocale& english_us() {
    static const CalendarLocale locale{
        "en-US", Weekday::sunday, english_names, "MMMM y", "MMM d, y", "EEEE, MMMM d, y", {"MM/dd/yyyy", U'/'}, false, "h:mm a", "HH:mm", english_strings,
    };
    return locale;
}

const CalendarLocale& english_gb() {
    static const CalendarLocale locale{
        "en-GB", Weekday::monday, english_names, "MMMM y", "d MMM y", "EEEE d MMMM y", {"dd/MM/yyyy", U'/'}, true, "h:mm a", "HH:mm", english_strings,
    };
    return locale;
}

const CalendarLocale& japanese() {
    static const CalendarLocale locale{
        "ja-JP", Weekday::sunday, japanese_names, "y年M月", "y年M月d日", "y年M月d日EEEE", {"yyyy/MM/dd", U'/'}, true, "a h:mm", "H:mm", japanese_strings,
    };
    return locale;
}

std::string input_placeholder(const InputPattern& pattern) {
    std::string out = pattern.pattern;
    for (char& c : out) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }
    return out;
}

}
