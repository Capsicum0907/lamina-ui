#include "lamina/widgets/date_picker.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/render/path.hpp"
#include "lamina/text/layout.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::DatePickerTokens& t = tokens::date_picker_tokens;
constexpr const tokens::DatePickerColors& k = tokens::date_picker_colors;
constexpr const tokens::DatePickerMotion& m = tokens::date_picker_motion;

text::TextStyle style_of(tokens::TypeRole role) {
    return tokens::text_style(tokens::type_style(role));
}

color::Argb dim(color::Argb c, double enabled) {
    return color::mix(color::scale_alpha(c, k.disabled_opacity), c, enabled);
}

const calendar::CalendarLocale& locale_or_default(const calendar::CalendarLocale* l) {
    return l ? *l : calendar::english_us();
}

double year_pitch() {
    return t.year_height + t.year_row_spacing;
}

double year_cell_width() {
    return month_grid_width() / t.year_columns;
}

}

std::string validate_date_input(const calendar::CalendarLocale& locale, const std::string& digits, const calendar::YearRange& years,
                                const SelectableDates& selectable, std::optional<calendar::Date>* parsed) {
    if (parsed) {
        parsed->reset();
    }
    const std::optional<calendar::Date> d = calendar::parse_input(locale.input, digits);
    if (!d) {
        return calendar::substitute(locale.strings.invalid_for_pattern, calendar::input_placeholder(locale.input));
    }
    if (!years.contains(d->year)) {
        return calendar::substitute(locale.strings.invalid_year_range, calendar::format_number(years.first), calendar::format_number(years.last));
    }
    if (!selectable.allows(*d)) {
        return calendar::substitute(locale.strings.invalid_not_allowed, calendar::format_date(locale.short_date_pattern, *d, locale.names));
    }
    if (parsed) {
        *parsed = d;
    }
    return {};
}

DatePicker::DatePicker(DatePickerOptions options, const text::FontStack& fonts, const icons::IconFont& icon_font)
    : options_(std::move(options)),
      fonts_(fonts),
      icon_font_(icon_font),
      locale_(&locale_or_default(options_.locale)),
      today_(options_.today),
      selected_(options_.selected),
      mode_(options_.mode),
      toggle_({{icon_font, options_.mode == DatePickerMode::picker ? t.input_mode_symbol : t.picker_mode_symbol},
               options_.mode == DatePickerMode::picker ? locale_->strings.switch_to_input_mode : locale_->strings.switch_to_calendar_mode,
               tokens::IconButtonStyle::standard, tokens::IconButtonSize::small, tokens::ButtonShape::round, tokens::IconButtonWidth::uniform,
               false, false, true, k.headline}),
      previous_button_({{icon_font, t.previous_symbol}, locale_->strings.previous_month, tokens::IconButtonStyle::standard,
                        tokens::IconButtonSize::small, tokens::ButtonShape::round, tokens::IconButtonWidth::uniform, false, false, true,
                        k.navigation}),
      next_button_({{icon_font, t.next_symbol}, locale_->strings.next_month, tokens::IconButtonStyle::standard, tokens::IconButtonSize::small,
                    tokens::ButtonShape::round, tokens::IconButtonWidth::uniform, false, false, true, k.navigation}),
      menu_{{}, motion::Animated(0.0, m.state_layer), {}},
      pointer_{{}, {}, {}, {}, motion::Animated(0.0, m.state_layer)},
      slide_(0.0, m.month_scroll),
      year_expand_(0.0, m.year_expand),
      year_alpha_(0.0, m.year_fade_in),
      mode_progress_(1.0, m.mode_slide),
      incoming_alpha_(1.0, m.mode_fade_in),
      outgoing_alpha_(0.0, m.mode_fade_out),
      height_(0.0, m.mode_slide),
      selection_fade_(1.0, m.selection),
      enabled_(options_.enabled) {
    if (selected_ && !options_.year_range.contains(selected_->year)) {
        selected_.reset();
    }
    calendar::YearMonth shown = options_.displayed_month.value_or(selected_ ? calendar::year_month(*selected_) : calendar::year_month(today_));
    if (!options_.year_range.contains(shown.year)) {
        shown = calendar::year_month(clamp_to_years(today_, options_.year_range));
    }
    displayed_ = shown;
    previous_ = shown;
    focus_date_ = calendar::first_day(shown);
    focus_year_ = shown.year;
    rebuild_input();
    height_.snap_to(content_height(mode_));
    if (!options_.enabled) {
        toggle_.set_enabled(false);
        previous_button_.set_enabled(false);
        next_button_.set_enabled(false);
        input_->set_enabled(false);
    }
    place_children();
}

void DatePicker::rebuild_input() {
    const std::string placeholder = calendar::input_placeholder(locale_->input);
    TextFieldOptions o{.label = locale_->strings.date_input_label,
                       .placeholder = placeholder,
                       .clipboard = options_.clipboard,
                       .enabled = enabled_.enabled(),
                       .accessible_name = locale_->strings.date_input_label + locale_->strings.list_separator + placeholder,
                       .filter = ascii_digit,
                       .mask = locale_->input.pattern};
    const bool had_focus = input_ && input_->focused();
    input_ = std::make_unique<TextField>(std::move(o), fonts_);
    input_->set_width(t.container_width - t.input_start - t.input_end);
    sync_input_from_selection();
    if (had_focus) {
        input_->set_focused(true);
    }
}

void DatePicker::sync_input_from_selection() {
    input_->set_text(selected_ ? calendar::input_digits(locale_->input, *selected_) : std::string());
    input_error_.clear();
    input_->set_supporting_text({});
    input_->set_error(false);
}

void DatePicker::set_locale(const calendar::CalendarLocale& locale) {
    locale_ = &locale;
    toggle_.set_accessible_name(mode_ == DatePickerMode::picker ? locale_->strings.switch_to_input_mode : locale_->strings.switch_to_calendar_mode);
    previous_button_.set_accessible_name(locale_->strings.previous_month);
    next_button_.set_accessible_name(locale_->strings.next_month);
    rebuild_input();
    place_children();
}

double DatePicker::width() const {
    return t.container_width;
}

double DatePicker::header_height() const {
    return options_.show_header ? t.header_height + t.divider_thickness : 0.0;
}

double DatePicker::picker_height() const {
    return t.navigation_height + t.cell_size + month_grid_height();
}

double DatePicker::input_height() const {
    return t.input_top + input_->height() + t.input_bottom;
}

double DatePicker::content_height(DatePickerMode mode) const {
    return mode == DatePickerMode::picker ? picker_height() : input_height();
}

double DatePicker::height() const {
    return header_height() + std::max(0.0, height_.value());
}

double DatePicker::target_height() const {
    return header_height() + content_height(mode_);
}

void DatePicker::set_origin(render::Point origin) {
    origin_ = origin;
    place_children();
}

layout::Size DatePicker::measure(const layout::Constraints&) const {
    return {width(), height()};
}

void DatePicker::arrange(const render::Rect& r) {
    set_origin({r.x, r.y});
}

render::Rect DatePicker::bounds() const {
    return {origin_.x, origin_.y, width(), height()};
}

double DatePicker::content_top() const {
    return origin_.y + header_height();
}

render::Point DatePicker::weekday_origin() const {
    return {origin_.x + t.horizontal_padding, content_top() + t.navigation_height};
}

render::Point DatePicker::grid_origin() const {
    return {origin_.x + t.horizontal_padding, content_top() + t.navigation_height + t.cell_size};
}

render::Rect DatePicker::years_area() const {
    const render::Point w = weekday_origin();
    return {w.x, w.y, month_grid_width(), t.cell_size * (t.rows + 1) - t.divider_thickness};
}

render::Rect DatePicker::content_top_left() const {
    return {origin_.x, content_top(), width(), height_.value()};
}

std::string DatePicker::month_label() const {
    return calendar::format_date(locale_->year_month_pattern, calendar::first_day(displayed_), locale_->names);
}

render::Rect DatePicker::menu_button_rect() const {
    const text::TextLine line = text::layout_line(fonts_, month_label(), style_of(t.month_button_type));
    const double w = 2.0 * t.month_button_padding + line.width + t.month_button_icon_spacing + t.month_button_icon_size;
    return {origin_.x + t.horizontal_padding, content_top() + (t.navigation_height - t.month_button_height) / 2.0, w, t.month_button_height};
}

void DatePicker::place_children() {
    const double slot = tokens::minimum_interactive_size;
    const double right = origin_.x + t.container_width - t.horizontal_padding;
    const double cy = content_top() + t.navigation_height / 2.0;
    next_button_.set_origin({right - slot / 2.0 - next_button_.width() / 2.0, cy - next_button_.height() / 2.0});
    previous_button_.set_origin({right - 1.5 * slot - previous_button_.width() / 2.0, cy - previous_button_.height() / 2.0});
    const double row_top = origin_.y + t.header_height - slot - t.toggle_bottom;
    const double toggle_cx = origin_.x + t.container_width - t.toggle_end - slot / 2.0;
    toggle_.set_origin({toggle_cx - toggle_.width() / 2.0, row_top + (slot - toggle_.height()) / 2.0});
    input_->set_origin({origin_.x + t.input_start, content_top() + t.input_top});
    layout_years();
}

void DatePicker::layout_years() {
    const int rows = (options_.year_range.count() + t.year_columns - 1) / t.year_columns;
    years_scroll_.set_viewport(years_area());
    years_scroll_.set_content_height(rows * t.year_height + std::max(0, rows - 1) * t.year_row_spacing);
}

render::Rect DatePicker::day_bounds(const calendar::Date& date) const {
    const render::Rect cell = day_rect(grid_origin(), calendar::cell_of(date, locale_->first_day_of_week));
    const double inset = (t.cell_size - t.day_size) / 2.0;
    return {cell.x + inset, cell.y + inset, t.day_size, t.day_size};
}

render::Rect DatePicker::year_bounds(int year) const {
    const int i = year - options_.year_range.first;
    const int row = i / t.year_columns;
    const int col = i % t.year_columns;
    const render::Rect area = years_area();
    const double cx = area.x + (col + 0.5) * year_cell_width();
    const double y = area.y + row * year_pitch() - years_scroll_.offset();
    return {cx - t.year_width / 2.0, y, t.year_width, t.year_height};
}

std::optional<calendar::Date> DatePicker::focused_day() const {
    if (!grid_focus_.focused()) {
        return std::nullopt;
    }
    return focus_date_;
}

std::optional<int> DatePicker::focused_year() const {
    if (!year_focus_.focused()) {
        return std::nullopt;
    }
    return focus_year_;
}

std::string DatePicker::title() const {
    return mode_ == DatePickerMode::picker ? locale_->strings.date_picker_title : locale_->strings.date_input_title;
}

std::string DatePicker::headline() const {
    if (selected_) {
        return calendar::format_date(locale_->short_date_pattern, *selected_, locale_->names);
    }
    return mode_ == DatePickerMode::picker ? locale_->strings.date_picker_headline : locale_->strings.date_input_headline;
}

std::string DatePicker::input_text() const {
    return input_->display_text();
}

void DatePicker::set_selected(std::optional<calendar::Date> date) {
    if (date && !options_.year_range.contains(date->year)) {
        date.reset();
    }
    if (date == selected_) {
        return;
    }
    selected_ = date;
    if (date) {
        fading_ = date;
        selection_fade_.snap_to(0.0);
        selection_fade_.animate_to(1.0);
    }
    sync_input_from_selection();
}

void DatePicker::set_displayed_month(calendar::YearMonth month, bool animate) {
    if (!options_.year_range.contains(month.year) || month == displayed_) {
        return;
    }
    const int delta = calendar::months_between(displayed_, month);
    previous_ = displayed_;
    displayed_ = month;
    if (animate && std::abs(delta) == 1) {
        slide_direction_ = delta;
        slide_.snap_to(static_cast<double>(delta) + slide_.value());
        slide_.animate_to(0.0);
    } else {
        slide_direction_ = 0;
        slide_.snap_to(0.0);
    }
    if (calendar::year_month(focus_date_) != displayed_) {
        const int last = calendar::days_in_month(displayed_.year, displayed_.month);
        focus_date_ = {displayed_.year, displayed_.month, std::min(focus_date_.day, last)};
    }
}

void DatePicker::set_mode(DatePickerMode mode) {
    if (mode == mode_) {
        return;
    }
    mode_ = mode;
    if (mode_ == DatePickerMode::picker && selected_) {
        set_displayed_month(calendar::year_month(*selected_), false);
    }
    if (mode_ == DatePickerMode::input && years_open_) {
        years_open_ = false;
        year_expand_.snap_to(0.0);
        year_alpha_.snap_to(0.0);
    }
    toggle_.set_icon({icon_font_, mode_ == DatePickerMode::picker ? t.input_mode_symbol : t.picker_mode_symbol});
    toggle_.set_accessible_name(mode_ == DatePickerMode::picker ? locale_->strings.switch_to_input_mode : locale_->strings.switch_to_calendar_mode);
    mode_progress_.snap_to(0.0);
    mode_progress_.animate_to(1.0);
    incoming_alpha_.snap_to(0.0);
    incoming_alpha_.animate_to(1.0);
    outgoing_alpha_.snap_to(1.0);
    outgoing_alpha_.animate_to(0.0);
    height_.animate_to(content_height(mode_));
    grid_focus_.set_focused(false);
    year_focus_.set_focused(false);
    menu_.focus.set_focused(false);
    if (mode_ == DatePickerMode::input) {
        input_focus_delay_ = t.input_focus_delay_seconds;
    } else {
        input_->set_focused(false);
        input_focus_delay_ = -1.0;
    }
}

void DatePicker::set_year_picker_open(bool open) {
    if (open == years_open_ || (open && mode_ != DatePickerMode::picker)) {
        return;
    }
    years_open_ = open;
    if (open) {
        year_expand_.snap_to(0.0);
        year_expand_.animate_to(1.0);
        year_alpha_.set_spec(m.year_fade_in);
        year_alpha_.snap_to(t.year_expand_from_alpha);
        year_alpha_.animate_to(1.0);
        layout_years();
        const int first_item = std::max(0, displayed_.year - options_.year_range.first - t.year_columns);
        years_scroll_.jump_to((first_item / t.year_columns) * year_pitch());
        focus_year_ = displayed_.year;
        grid_focus_.set_focused(false);
    } else {
        year_expand_.animate_to(0.0);
        year_alpha_.set_spec(m.year_fade_out);
        year_alpha_.animate_to(0.0);
        year_focus_.set_focused(false);
    }
}

void DatePicker::pick_year(int year) {
    if (!options_.year_range.contains(year) || !options_.selectable.allows_year(year)) {
        return;
    }
    const bool keyboard = year_focus_.focused();
    set_displayed_month({year, displayed_.month}, false);
    set_year_picker_open(false);
    if (keyboard) {
        focus_day(focus_date_);
        grid_focus_.set_focused(true);
    }
}

void DatePicker::focus_day(const calendar::Date& date) {
    const calendar::Date d = clamp_to_years(date, options_.year_range);
    focus_date_ = d;
    if (calendar::year_month(d) != displayed_) {
        set_displayed_month(calendar::year_month(d));
    }
}

void DatePicker::select(const calendar::Date& date, DatePickerResponse& response) {
    MonthGridState s = grid_state(false);
    if (!day_enabled(s, date) || !enabled()) {
        return;
    }
    response.consumed = true;
    if (selected_ != date) {
        set_selected(date);
        response.changed = true;
    }
}

MonthGridState DatePicker::grid_state(bool with_pointer) const {
    MonthGridState s{locale_, today_, {selected_, std::nullopt, false}, &options_.selectable, options_.year_range};
    if (with_pointer) {
        s.hovered = pointer_.pressed_day ? pointer_.pressed_day : pointer_.hovered_day;
        s.hover_opacity = pointer_.state_layer.value();
    }
    s.focused = focus_date_;
    s.focus = &grid_focus_;
    s.fading = fading_;
    s.fade = selection_fade_.value();
    s.enabled = enabled_.amount();
    return s;
}

void DatePicker::add_focus_stops(FocusManager& focus) {
    auto add = [&](FocusStop::Hooks hooks) {
        stops_.push_back(std::make_unique<FocusStop>(std::move(hooks)));
        focus.add(*stops_.back());
    };
    add(forward_focus(toggle_, [this] { return options_.show_header && options_.show_mode_toggle; }));
    add({[this] { return menu_.focus.focused(); }, [this](bool f) { menu_.focus.set_focused(f && enabled() && picker_shown()); },
         [this] { return enabled() && picker_shown(); }, [this] { return menu_button_rect(); }});
    add(forward_focus(previous_button_, [this] { return days_shown(); }));
    add(forward_focus(next_button_, [this] { return days_shown(); }));
    add({[this] { return grid_focus_.focused(); },
         [this](bool f) {
             if (f && !grid_focus_.focused()) {
                 if (selected_ && calendar::year_month(*selected_) == displayed_) {
                     focus_date_ = *selected_;
                 } else if (calendar::year_month(today_) == displayed_) {
                     focus_date_ = today_;
                 } else if (calendar::year_month(focus_date_) != displayed_) {
                     focus_date_ = calendar::first_day(displayed_);
                 }
             }
             grid_focus_.set_focused(f && enabled() && days_shown());
         },
         [this] { return enabled() && days_shown(); }, [this] { return day_bounds(focus_date_); }});
    add({[this] { return year_focus_.focused(); },
         [this](bool f) {
             if (f && !year_focus_.focused()) {
                 focus_year_ = displayed_.year;
                 const render::Rect b = year_bounds(focus_year_);
                 const double top = b.y - years_area().y + years_scroll_.offset();
                 years_scroll_.ensure_visible(top, top + t.year_height);
             }
             year_focus_.set_focused(f && enabled() && years_open_);
         },
         [this] { return enabled() && picker_shown() && years_open_; }, [this] { return year_bounds(focus_year_); }});
    add(forward_focus(*input_, [this] { return mode_ == DatePickerMode::input; }));
}

bool DatePicker::focused() const {
    return toggle_.focused() || previous_button_.focused() || next_button_.focused() || menu_.focus.focused() || grid_focus_.focused() ||
           year_focus_.focused() || input_->focused();
}

void DatePicker::clear_focus() {
    toggle_.set_focused(false);
    previous_button_.set_focused(false);
    next_button_.set_focused(false);
    menu_.focus.set_focused(false);
    grid_focus_.set_focused(false);
    year_focus_.set_focused(false);
    input_->set_focused(false);
}

void DatePicker::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    toggle_.set_enabled(enabled);
    previous_button_.set_enabled(enabled);
    next_button_.set_enabled(enabled);
    input_->set_enabled(enabled);
    if (!enabled) {
        clear_focus();
        cancel_interaction();
    }
}

AccessibleNode DatePicker::accessible() const {
    AccessibleNode node{AccessibleRole::group, title(), bounds(), {.enabled = enabled()}};
    if (options_.show_header) {
        const std::string verbose =
            selected_ ? calendar::format_date(locale_->long_date_pattern, *selected_, locale_->names) : locale_->strings.no_selection_description;
        const std::string& tmpl = mode_ == DatePickerMode::picker ? locale_->strings.date_picker_headline_description
                                                                  : locale_->strings.date_input_headline_description;
        node.children.push_back({AccessibleRole::status, headline(), {origin_.x, origin_.y, width(), t.header_height}, {}, {},
                                 calendar::substitute(tmpl, verbose)});
        if (options_.show_mode_toggle) {
            node.children.push_back(toggle_.accessible());
        }
    }
    if (mode_ == DatePickerMode::input) {
        node.children.push_back(input_->accessible());
        return node;
    }
    node.children.push_back({AccessibleRole::button, month_label(), menu_button_rect(),
                             {.enabled = enabled(), .focusable = enabled(), .focused = menu_.focus.focused()}, {},
                             years_open_ ? locale_->strings.switch_to_day_selection : locale_->strings.switch_to_year_selection});
    if (years_open_) {
        AccessibleNode years{AccessibleRole::group, locale_->strings.year_picker_pane_title, years_area(), {.enabled = enabled()}};
        const render::Rect area = years_area();
        for (int y = options_.year_range.first; y <= options_.year_range.last; ++y) {
            const render::Rect b = year_bounds(y);
            if (b.y + b.height < area.y || b.y > area.y + area.height) {
                continue;
            }
            years.children.push_back({AccessibleRole::button, calendar::substitute(locale_->strings.navigate_to_year, calendar::format_number(y)), b,
                                      {.enabled = enabled() && options_.selectable.allows_year(y),
                                       .focused = year_focus_.focused() && focus_year_ == y,
                                       .selected = y == displayed_.year}});
        }
        node.children.push_back(std::move(years));
        return node;
    }
    node.children.push_back(previous_button_.accessible());
    node.children.push_back(next_button_.accessible());
    AccessibleNode grid{AccessibleRole::group, month_label(), {grid_origin().x, weekday_origin().y, month_grid_width(), t.cell_size + month_grid_height()},
                        {.enabled = enabled(), .focusable = enabled(), .focused = grid_focus_.focused()}};
    for (AccessibleNode& w : weekday_accessible(*locale_, weekday_origin())) {
        grid.children.push_back(std::move(w));
    }
    month_accessible(grid.children, grid_origin(), displayed_, grid_state(false));
    node.children.push_back(std::move(grid));
    return node;
}

void DatePicker::on_input_changed(DatePickerResponse& response) {
    const std::string digits = input_->text();
    std::optional<calendar::Date> parsed;
    std::string error;
    if (digits.size() == calendar::input_pattern_digits(locale_->input)) {
        error = validate_date_input(*locale_, digits, options_.year_range, options_.selectable, &parsed);
    }
    if (error != input_error_) {
        input_error_ = error;
        input_->set_supporting_text(error);
        input_->set_error(!error.empty());
        height_.animate_to(content_height(mode_));
    }
    if (parsed != selected_) {
        selected_ = parsed;
        if (parsed) {
            displayed_ = calendar::year_month(*parsed);
            previous_ = displayed_;
        }
        response.changed = true;
    }
}

DatePickerResponse DatePicker::handle(const input::PointerEvent& e) {
    DatePickerResponse response;
    menu_.focus.release_on_press(e);
    grid_focus_.release_on_press(e);
    year_focus_.release_on_press(e);
    if (!enabled()) {
        return response;
    }
    if (options_.show_header && options_.show_mode_toggle && toggle_.handle(e)) {
        set_mode(mode_ == DatePickerMode::picker ? DatePickerMode::input : DatePickerMode::picker);
        response.consumed = true;
        return response;
    }
    if (mode_ == DatePickerMode::input) {
        const std::string before = input_->text();
        if (input_->handle(e)) {
            response.consumed = true;
        }
        if (input_->text() != before) {
            on_input_changed(response);
        }
        return response;
    }
    if (menu_.press.update(e, touch_target(menu_button_rect()))) {
        set_year_picker_open(!years_open_);
        response.consumed = true;
    }
    menu_.state_layer.animate_to(menu_.press.state_layer_opacity());
    if (years_open_) {
        if (years_scroll_.handle(e)) {
            response.consumed = true;
            return response;
        }
        std::optional<int> over;
        const render::Rect area = years_area();
        if (e.action != input::PointerAction::leave && contains(area, e.x_dp, e.y_dp)) {
            for (int y = options_.year_range.first; y <= options_.year_range.last; ++y) {
                if (contains(year_bounds(y), e.x_dp, e.y_dp)) {
                    over = y;
                    break;
                }
            }
        }
        pointer_.hovered_year = over;
        if (e.action == input::PointerAction::down) {
            pointer_.pressed_year = over;
        } else if (e.action == input::PointerAction::up) {
            if (over && over == pointer_.pressed_year) {
                pick_year(*over);
                response.consumed = true;
            }
            pointer_.pressed_year.reset();
        } else if (e.action == input::PointerAction::leave) {
            pointer_.pressed_year.reset();
        }
        pointer_.state_layer.animate_to(pointer_.pressed_year ? tokens::state_layer_opacity::pressed
                                        : over                ? tokens::state_layer_opacity::hover
                                                              : 0.0);
        if (contains(area, e.x_dp, e.y_dp)) {
            response.consumed = true;
        }
        return response;
    }
    if (previous_button_.handle(e)) {
        set_displayed_month(calendar::add_months(displayed_, -1));
        response.consumed = true;
    }
    if (next_button_.handle(e)) {
        set_displayed_month(calendar::add_months(displayed_, 1));
        response.consumed = true;
    }
    std::optional<calendar::Date> over;
    if (e.action != input::PointerAction::leave) {
        over = day_at(grid_origin(), displayed_, locale_->first_day_of_week, e.x_dp, e.y_dp);
        if (over && !contains(day_bounds(*over), e.x_dp, e.y_dp)) {
            const render::Rect cell = day_rect(grid_origin(), calendar::cell_of(*over, locale_->first_day_of_week));
            if (!contains(cell, e.x_dp, e.y_dp)) {
                over.reset();
            }
        }
        if (over && !day_enabled(grid_state(false), *over)) {
            over.reset();
        }
    }
    pointer_.hovered_day = over;
    switch (e.action) {
    case input::PointerAction::down:
        pointer_.pressed_day = over;
        break;
    case input::PointerAction::up:
        if (over && over == pointer_.pressed_day) {
            focus_date_ = *over;
            select(*over, response);
        }
        pointer_.pressed_day.reset();
        break;
    case input::PointerAction::leave:
        pointer_.pressed_day.reset();
        break;
    case input::PointerAction::move:
        break;
    }
    pointer_.state_layer.animate_to(pointer_.pressed_day ? tokens::state_layer_opacity::pressed
                                    : over               ? tokens::state_layer_opacity::hover
                                                         : 0.0);
    if (over) {
        response.consumed = true;
    }
    return response;
}

DatePickerResponse DatePicker::handle(const input::KeyEvent& e) {
    DatePickerResponse response;
    if (!enabled()) {
        return response;
    }
    if (toggle_.focused()) {
        if (toggle_.handle(e)) {
            set_mode(mode_ == DatePickerMode::picker ? DatePickerMode::input : DatePickerMode::picker);
            response.consumed = true;
        }
        return response;
    }
    if (input_->focused()) {
        const std::string before = input_->text();
        input_->handle(e);
        response.consumed = e.key != input::Key::escape && e.key != input::Key::tab;
        if (input_->text() != before) {
            on_input_changed(response);
        }
        return response;
    }
    if (menu_.focus.activates(e, KeyActivation::space_or_enter)) {
        const bool opening = !years_open_;
        set_year_picker_open(opening);
        if (opening) {
            menu_.focus.set_focused(false);
            year_focus_.set_focused(true);
            const render::Rect b = year_bounds(focus_year_);
            const double top = b.y - years_area().y + years_scroll_.offset();
            years_scroll_.ensure_visible(top, top + t.year_height);
        }
        response.consumed = true;
        return response;
    }
    if (previous_button_.focused() && previous_button_.handle(e)) {
        set_displayed_month(calendar::add_months(displayed_, -1));
        response.consumed = true;
        return response;
    }
    if (next_button_.focused() && next_button_.handle(e)) {
        set_displayed_month(calendar::add_months(displayed_, 1));
        response.consumed = true;
        return response;
    }
    if (grid_focus_.focused()) {
        if (const std::optional<calendar::Date> next = grid_key_target(e, focus_date_, locale_->first_day_of_week)) {
            focus_day(*next);
            response.consumed = true;
            return response;
        }
        if (grid_focus_.activates(e, KeyActivation::space_or_enter)) {
            select(focus_date_, response);
            response.consumed = true;
        }
        return response;
    }
    if (year_focus_.focused()) {
        int next = focus_year_;
        switch (e.key) {
        case input::Key::left: next -= 1; break;
        case input::Key::right: next += 1; break;
        case input::Key::up: next -= t.year_columns; break;
        case input::Key::down: next += t.year_columns; break;
        case input::Key::home: next = options_.year_range.first; break;
        case input::Key::end: next = options_.year_range.last; break;
        default:
            if (year_focus_.activates(e, KeyActivation::space_or_enter)) {
                pick_year(focus_year_);
                response.consumed = true;
            }
            return response;
        }
        focus_year_ = std::clamp(next, options_.year_range.first, options_.year_range.last);
        const render::Rect b = year_bounds(focus_year_);
        const double top = b.y - years_area().y + years_scroll_.offset();
        years_scroll_.ensure_visible(top, top + t.year_height);
        response.consumed = true;
    }
    return response;
}

void DatePicker::handle(const input::TextEvent& e) {
    if (!input_->focused()) {
        return;
    }
    const std::string before = input_->text();
    input_->handle(e);
    if (input_->text() != before) {
        DatePickerResponse ignored;
        on_input_changed(ignored);
    }
}

bool DatePicker::handle(const input::WheelEvent& e) {
    if (!enabled() || !years_open_) {
        return false;
    }
    return years_scroll_.handle(e);
}

void DatePicker::cancel_interaction() {
    toggle_.cancel_interaction();
    previous_button_.cancel_interaction();
    next_button_.cancel_interaction();
    input_->cancel_interaction();
    years_scroll_.cancel_interaction();
    menu_.press = {};
    menu_.state_layer.animate_to(0.0);
    pointer_.hovered_day.reset();
    pointer_.pressed_day.reset();
    pointer_.hovered_year.reset();
    pointer_.pressed_year.reset();
    pointer_.state_layer.animate_to(0.0);
}

void DatePicker::step(double dt) {
    toggle_.step(dt);
    previous_button_.step(dt);
    next_button_.step(dt);
    input_->step(dt);
    menu_.state_layer.step(dt);
    menu_.focus.step(dt);
    grid_focus_.step(dt);
    year_focus_.step(dt);
    pointer_.state_layer.step(dt);
    years_scroll_.step(dt);
    slide_.step(dt);
    year_expand_.step(dt);
    year_alpha_.step(dt);
    mode_progress_.step(dt);
    incoming_alpha_.step(dt);
    outgoing_alpha_.step(dt);
    height_.step(dt);
    selection_fade_.step(dt);
    enabled_.step(dt);
    if (input_focus_delay_ >= 0.0) {
        input_focus_delay_ -= dt;
        if (input_focus_delay_ < 0.0 && mode_ == DatePickerMode::input) {
            toggle_.set_focused(false);
            input_->set_focused(true);
        }
    }
    place_children();
}

bool DatePicker::animating() const {
    return toggle_.animating() || previous_button_.animating() || next_button_.animating() || input_->animating() ||
           !menu_.state_layer.settled() || !menu_.focus.settled() || !grid_focus_.settled() || !year_focus_.settled() ||
           !pointer_.state_layer.settled() || years_scroll_.animating() || !slide_.settled() || !year_expand_.settled() ||
           !year_alpha_.settled() || !mode_progress_.settled() || !incoming_alpha_.settled() || !outgoing_alpha_.settled() ||
           !height_.settled() || !selection_fade_.settled() || !enabled_.settled() || input_focus_delay_ >= 0.0;
}

std::optional<render::Rect> DatePicker::caret_area() const {
    if (!input_->focused()) {
        return std::nullopt;
    }
    return input_->caret_area();
}

std::optional<double> DatePicker::next_wake_seconds() const {
    std::optional<double> wake;
    if (input_->focused()) {
        wake = input_->next_blink_seconds();
    }
    const double scroll = years_scroll_.next_wake_seconds();
    if (scroll >= 0.0) {
        wake = wake ? std::min(*wake, scroll) : scroll;
    }
    return wake;
}

void DatePicker::paint_header(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double e = enabled_.amount();
    const text::TextLine title_line = text::layout_line(fonts_, title(), style_of(t.title_type));
    text::draw_line(canvas, fonts_, title_line, {origin_.x + t.header_start, origin_.y + t.title_top}, dim(scheme[k.title], e));
    const text::TextLine headline_line = text::layout_line(fonts_, headline(), style_of(t.headline_type));
    const double slot = tokens::minimum_interactive_size;
    const double row_top = origin_.y + t.header_height - slot - t.toggle_bottom;
    const double headline_y = row_top + (slot + t.toggle_bottom - t.headline_bottom - headline_line.box_height()) / 2.0;
    canvas.push_clip({origin_.x + t.header_start, origin_.y, t.container_width - t.header_start - t.header_end - slot, t.header_height});
    text::draw_line(canvas, fonts_, headline_line, {origin_.x + t.header_start, headline_y}, dim(scheme[k.headline], e));
    canvas.pop_clip();
    if (options_.show_mode_toggle) {
        toggle_.paint(canvas, scheme);
    }
    canvas.fill_rect({origin_.x, origin_.y + t.header_height, t.container_width, t.divider_thickness}, scheme[k.divider]);
}

void DatePicker::paint_picker(render::Canvas& canvas, const color::ColorScheme& scheme, double dy) const {
    const double e = enabled_.amount();
    canvas.save();
    canvas.translate(0.0, dy);
    const render::Rect menu = menu_button_rect();
    const color::Argb nav = dim(scheme[k.navigation], e);
    if (menu_.state_layer.value() > 0.0) {
        canvas.fill_path(render::rounded_rect_path(menu, render::uniform_radii(menu.height / 2.0)), color::with_alpha(nav, menu_.state_layer.value()));
    }
    const text::TextLine label = text::layout_line(fonts_, month_label(), style_of(t.month_button_type));
    text::draw_line(canvas, fonts_, label, {menu.x + t.month_button_padding, menu.y + (menu.height - label.box_height()) / 2.0}, nav);
    icon_font_.draw(canvas, years_open_ ? t.menu_open_symbol : t.menu_symbol,
                    {menu.x + t.month_button_padding + label.width + t.month_button_icon_spacing, menu.y + (menu.height - t.month_button_icon_size) / 2.0},
                    t.month_button_icon_size, nav);
    menu_.focus.paint(canvas, scheme, menu, menu.height / 2.0);
    const double days_alpha = 1.0 - std::clamp(year_expand_.value(), 0.0, 1.0);
    if (!years_open_ || days_alpha > 0.0) {
        if (!years_open_) {
            previous_button_.paint(canvas, scheme);
            next_button_.paint(canvas, scheme);
        }
        paint_weekdays(canvas, scheme, fonts_, *locale_, weekday_origin());
        const render::Point g = grid_origin();
        canvas.push_clip({g.x, g.y, month_grid_width(), month_grid_height()});
        const double w = month_grid_width();
        const double s = slide_.value();
        MonthGridState state = grid_state(true);
        if (slide_direction_ != 0 && std::abs(s) > 1e-4) {
            MonthGridState old = state;
            old.hovered.reset();
            paint_month(canvas, scheme, fonts_, {g.x + (s - slide_direction_) * w, g.y}, previous_, old);
        }
        paint_month(canvas, scheme, fonts_, {g.x + s * w, g.y}, displayed_, state);
        canvas.pop_clip();
    }
    canvas.restore();
    if (year_expand_.value() > 0.0 || year_alpha_.value() > 0.0) {
        canvas.save();
        canvas.translate(0.0, dy);
        paint_years(canvas, scheme);
        canvas.restore();
    }
}

void DatePicker::paint_years(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect area = years_area();
    const double full = area.height + t.divider_thickness;
    const double h = full * std::clamp(year_expand_.value(), 0.0, 1.0);
    if (h <= 0.0) {
        return;
    }
    const color::ColorScheme faded = color::faded(scheme, std::clamp(year_alpha_.value(), 0.0, 1.0));
    canvas.push_clip({origin_.x, area.y, t.container_width, h});
    canvas.save();
    canvas.translate(0.0, h - full);
    canvas.fill_rect({origin_.x, area.y, t.container_width, full}, faded[k.container]);
    canvas.push_clip(area);
    const double e = enabled_.amount();
    const text::TextStyle style = style_of(t.year_type);
    for (int y = options_.year_range.first; y <= options_.year_range.last; ++y) {
        const render::Rect b = year_bounds(y);
        if (b.y + b.height < area.y || b.y > area.y + area.height) {
            continue;
        }
        const bool allowed = options_.selectable.allows_year(y);
        const double ye = allowed ? e : 0.0;
        const bool selected = y == displayed_.year;
        const bool current = y == today_.year;
        color::Argb content = selected ? faded[k.selected_year] : current ? faded[k.current_year] : faded[k.year];
        content = dim(content, ye);
        const render::Path pill = render::rounded_rect_path(b, render::uniform_radii(b.height / 2.0));
        if (selected) {
            canvas.fill_path(pill, dim(faded[k.selected_year_container], ye));
        } else if (current) {
            canvas.fill_path(render::ring_path(b, b.height / 2.0, t.today_outline), dim(faded[k.today_outline], ye), render::FillRule::even_odd);
        }
        const bool hovered = (pointer_.pressed_year ? pointer_.pressed_year : pointer_.hovered_year) == y;
        if (hovered && allowed && pointer_.state_layer.value() > 0.0) {
            canvas.fill_path(pill, color::with_alpha(content, pointer_.state_layer.value()));
        }
        const text::TextLine line = text::layout_line(fonts_, calendar::format_number(y), style);
        text::draw_line(canvas, fonts_, line, {b.x + (b.width - line.width) / 2.0, b.y + (b.height - line.box_height()) / 2.0}, content);
        if (year_focus_.focused() && focus_year_ == y) {
            year_focus_.paint(canvas, faded, b, b.height / 2.0);
        }
    }
    canvas.pop_clip();
    canvas.fill_rect({origin_.x, area.y + area.height, t.container_width, t.divider_thickness}, faded[k.divider]);
    canvas.restore();
    canvas.pop_clip();
}

void DatePicker::paint_input(render::Canvas& canvas, const color::ColorScheme& scheme, double dy) const {
    canvas.save();
    canvas.translate(0.0, dy);
    input_->paint(canvas, scheme);
    canvas.restore();
}

void DatePicker::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect b = bounds();
    canvas.fill_rect(b, scheme[k.container]);
    if (options_.show_header) {
        paint_header(canvas, scheme);
    }
    const double top = content_top();
    canvas.push_clip({b.x, top, b.width, std::max(0.0, b.y + b.height - top)});
    const double p = mode_progress_.value();
    const double in_alpha = std::clamp(incoming_alpha_.value(), 0.0, 1.0);
    const double out_alpha = std::clamp(outgoing_alpha_.value(), 0.0, 1.0);
    const bool to_input = mode_ == DatePickerMode::input;
    const double input_h = input_height();
    if (out_alpha > 0.0) {
        const color::ColorScheme faded = color::faded(scheme, out_alpha);
        if (to_input) {
            paint_picker(canvas, faded, -t.switch_parallax * p);
        } else {
            paint_input(canvas, faded, input_h * p);
        }
    }
    const color::ColorScheme faded_in = in_alpha < 1.0 ? color::faded(scheme, in_alpha) : scheme;
    if (to_input) {
        paint_input(canvas, faded_in, input_h * (1.0 - p));
    } else {
        paint_picker(canvas, faded_in, -t.switch_parallax * (1.0 - p));
    }
    canvas.pop_clip();
}

}
