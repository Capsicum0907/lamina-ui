#include "lamina/widgets/date_range_picker.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/widgets/interaction.hpp"

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

double field_width() {
    return (t.container_width - t.input_start - t.input_end - t.range_input_spacing) / 2.0;
}

}

DateRangePicker::DateRangePicker(DateRangePickerOptions options, const text::FontStack& fonts, const icons::IconFont& icon_font)
    : options_(std::move(options)),
      fonts_(fonts),
      icon_font_(icon_font),
      locale_(options_.locale ? options_.locale : &calendar::english_us()),
      mode_(options_.mode),
      toggle_({{icon_font, options_.mode == DatePickerMode::picker ? t.input_mode_symbol : t.picker_mode_symbol},
               options_.mode == DatePickerMode::picker ? locale_->strings.switch_to_input_mode : locale_->strings.switch_to_calendar_mode,
               tokens::IconButtonStyle::standard, tokens::IconButtonSize::small, tokens::ButtonShape::round, tokens::IconButtonWidth::uniform,
               false, false, true, k.headline}),
      state_layer_(0.0, m.state_layer),
      mode_alpha_(1.0, m.range_crossfade),
      selection_fade_(1.0, m.selection),
      enabled_(options_.enabled) {
    set_selection(options_.start, options_.end);
    focus_date_ = start_.value_or(options_.today);
    rebuild_inputs();
    place_children();
    calendar::YearMonth shown = options_.displayed_month.value_or(start_ ? calendar::year_month(*start_) : calendar::year_month(options_.today));
    if (!options_.year_range.contains(shown.year)) {
        shown = calendar::year_month(clamp_to_years(options_.today, options_.year_range));
    }
    show_month(shown, false);
    if (!options_.enabled) {
        toggle_.set_enabled(false);
    }
}

void DateRangePicker::rebuild_inputs() {
    const std::string placeholder = calendar::input_placeholder(locale_->input);
    const std::string labels[2] = {locale_->strings.range_start_headline, locale_->strings.range_end_headline};
    for (std::size_t i = 0; i < 2; ++i) {
        const bool had_focus = fields_[i] && fields_[i]->focused();
        TextFieldOptions o{.label = labels[i],
                           .placeholder = placeholder,
                           .clipboard = options_.clipboard,
                           .enabled = enabled_.enabled(),
                           .accessible_name = labels[i] + locale_->strings.list_separator + placeholder,
                           .filter = ascii_digit,
                           .mask = locale_->input.pattern,
                           .min_width = field_width()};
        fields_[i] = std::make_unique<TextField>(std::move(o), fonts_);
        fields_[i]->set_width(field_width());
        if (had_focus) {
            fields_[i]->set_focused(true);
        }
    }
    sync_inputs();
}

void DateRangePicker::sync_inputs() {
    const std::optional<calendar::Date> values[2] = {start_, end_};
    for (std::size_t i = 0; i < 2; ++i) {
        fields_[i]->set_text(values[i] ? calendar::input_digits(locale_->input, *values[i]) : std::string());
        errors_[i].clear();
        fields_[i]->set_supporting_text({});
        fields_[i]->set_error(false);
    }
}

void DateRangePicker::set_locale(const calendar::CalendarLocale& locale) {
    locale_ = &locale;
    toggle_.set_accessible_name(mode_ == DatePickerMode::picker ? locale_->strings.switch_to_input_mode : locale_->strings.switch_to_calendar_mode);
    rebuild_inputs();
    place_children();
}

double DateRangePicker::width() const {
    return t.container_width;
}

void DateRangePicker::set_height(double height) {
    options_.height = height;
    place_children();
}

double DateRangePicker::header_height() const {
    const double slot = tokens::minimum_interactive_size;
    const double row = std::max(style_of(t.range_headline_type).line_height + t.headline_bottom, slot + t.toggle_bottom);
    return std::max(t.range_header_height, t.title_top + style_of(t.title_type).line_height + row) + t.divider_thickness;
}

double DateRangePicker::content_top() const {
    return origin_.y + header_height();
}

double DateRangePicker::item_height() const {
    return t.subhead_top + style_of(t.subhead_type).line_height + t.subhead_bottom + month_grid_height();
}

render::Rect DateRangePicker::list_area() const {
    const double top = content_top() + t.cell_size;
    return {origin_.x, top, t.container_width, std::max(0.0, origin_.y + options_.height - top)};
}

int DateRangePicker::month_count() const {
    return options_.year_range.count() * calendar::months_per_year;
}

int DateRangePicker::month_index(calendar::YearMonth month) const {
    return calendar::months_between({options_.year_range.first, 1}, month);
}

calendar::YearMonth DateRangePicker::month_at(int index) const {
    return calendar::add_months(calendar::YearMonth{options_.year_range.first, 1}, index);
}

render::Point DateRangePicker::grid_origin(int index) const {
    const render::Rect area = list_area();
    const double top = area.y + index * item_height() - list_.offset();
    return {origin_.x + t.horizontal_padding, top + item_height() - month_grid_height()};
}

calendar::YearMonth DateRangePicker::first_visible_month() const {
    const int index = std::clamp(static_cast<int>(std::floor(list_.offset() / item_height() + 1e-6)), 0, month_count() - 1);
    return month_at(index);
}

void DateRangePicker::show_month(calendar::YearMonth month, bool animate) {
    const double target = std::clamp(month_index(month), 0, month_count() - 1) * item_height();
    if (animate) {
        list_.scroll_to(target);
    } else {
        list_.jump_to(target);
    }
}

render::Rect DateRangePicker::day_bounds(const calendar::Date& date) const {
    const render::Rect cell =
        day_rect(grid_origin(month_index(calendar::year_month(date))), calendar::cell_of(date, locale_->first_day_of_week));
    const double inset = (t.cell_size - t.day_size) / 2.0;
    return {cell.x + inset, cell.y + inset, t.day_size, t.day_size};
}

void DateRangePicker::set_origin(render::Point origin) {
    origin_ = origin;
    place_children();
}

layout::Size DateRangePicker::measure(const layout::Constraints&) const {
    return {width(), height()};
}

void DateRangePicker::arrange(const render::Rect& r) {
    set_origin({r.x, r.y});
}

render::Rect DateRangePicker::bounds() const {
    return {origin_.x, origin_.y, width(), height()};
}

void DateRangePicker::place_children() {
    const double slot = tokens::minimum_interactive_size;
    const double row_top = origin_.y + header_height() - t.divider_thickness - slot - t.toggle_bottom;
    const double cx = origin_.x + t.container_width - t.toggle_end - slot / 2.0;
    toggle_.set_origin({cx - toggle_.width() / 2.0, row_top + (slot - toggle_.height()) / 2.0});
    fields_[0]->set_origin({origin_.x + t.input_start, content_top() + t.input_top});
    fields_[1]->set_origin({origin_.x + t.input_start + field_width() + t.range_input_spacing, content_top() + t.input_top});
    list_.set_viewport(list_area());
    list_.set_content_height(month_count() * item_height());
}

std::optional<calendar::Date> DateRangePicker::focused_day() const {
    return grid_focus_.focused() ? std::optional<calendar::Date>(focus_date_) : std::nullopt;
}

std::string DateRangePicker::title() const {
    return mode_ == DatePickerMode::picker ? locale_->strings.range_picker_title : locale_->strings.range_input_title;
}

std::string DateRangePicker::headline() const {
    const std::string a = start_ ? calendar::format_date(locale_->short_date_pattern, *start_, locale_->names) : locale_->strings.range_start_headline;
    const std::string b = end_ ? calendar::format_date(locale_->short_date_pattern, *end_, locale_->names) : locale_->strings.range_end_headline;
    return a + " " + locale_->strings.range_delimiter + " " + b;
}

void DateRangePicker::set_selection(std::optional<calendar::Date> start, std::optional<calendar::Date> end) {
    if (start && !options_.year_range.contains(start->year)) {
        start.reset();
    }
    if (end && !options_.year_range.contains(end->year)) {
        end.reset();
    }
    if (!start || (end && *end < *start)) {
        start.reset();
        end.reset();
    }
    if (start && start != start_) {
        fading_ = start;
        selection_fade_.snap_to(0.0);
        selection_fade_.animate_to(1.0);
    }
    start_ = start;
    end_ = end;
}

void DateRangePicker::set_mode(DatePickerMode mode) {
    if (mode == mode_) {
        return;
    }
    mode_ = mode;
    toggle_.set_icon({icon_font_, mode_ == DatePickerMode::picker ? t.input_mode_symbol : t.picker_mode_symbol});
    toggle_.set_accessible_name(mode_ == DatePickerMode::picker ? locale_->strings.switch_to_input_mode : locale_->strings.switch_to_calendar_mode);
    mode_alpha_.snap_to(0.0);
    mode_alpha_.animate_to(1.0);
    grid_focus_.set_focused(false);
    if (mode_ == DatePickerMode::picker) {
        fields_[0]->set_focused(false);
        fields_[1]->set_focused(false);
        if (start_) {
            show_month(calendar::year_month(*start_), false);
        }
    } else {
        sync_inputs();
    }
}

MonthGridState DateRangePicker::grid_state(bool with_pointer) const {
    MonthGridState s{locale_, options_.today, {start_, end_, true}, &options_.selectable, options_.year_range};
    if (with_pointer) {
        s.hovered = pressed_ ? pressed_ : hovered_;
        s.hover_opacity = state_layer_.value();
    }
    s.focused = focus_date_;
    s.focus = &grid_focus_;
    s.fading = fading_;
    s.fade = selection_fade_.value();
    s.enabled = enabled_.amount();
    return s;
}

void DateRangePicker::click(const calendar::Date& date, DatePickerResponse& response) {
    if (!enabled() || !day_enabled(grid_state(false), date)) {
        return;
    }
    response.consumed = true;
    response.changed = true;
    if (!start_ || end_) {
        set_selection(date, std::nullopt);
    } else if (date >= *start_) {
        set_selection(start_, date);
    } else {
        set_selection(date, std::nullopt);
    }
}

void DateRangePicker::focus_day(const calendar::Date& date) {
    focus_date_ = clamp_to_years(date, options_.year_range);
    const int index = month_index(calendar::year_month(focus_date_));
    const double item_top = index * item_height();
    const render::Rect cell = day_rect({0.0, item_top + item_height() - month_grid_height()}, calendar::cell_of(focus_date_, locale_->first_day_of_week));
    list_.ensure_visible(cell.y - (item_height() - month_grid_height()), cell.y + cell.height);
}

void DateRangePicker::add_focus_stops(FocusManager& focus) {
    auto add = [&](FocusStop::Hooks hooks) {
        stops_.push_back(std::make_unique<FocusStop>(std::move(hooks)));
        focus.add(*stops_.back());
    };
    add(forward_focus(toggle_, [this] { return options_.show_mode_toggle; }));
    add({[this] { return grid_focus_.focused(); },
         [this](bool f) {
             if (f && !grid_focus_.focused()) {
                 const calendar::YearMonth top = first_visible_month();
                 focus_date_ = start_ && calendar::year_month(*start_) >= top ? *start_ : calendar::first_day(top);
                 focus_day(focus_date_);
             }
             grid_focus_.set_focused(f && enabled() && mode_ == DatePickerMode::picker);
         },
         [this] { return enabled() && mode_ == DatePickerMode::picker; }, [this] { return day_bounds(focus_date_); }});
    for (std::size_t i = 0; i < 2; ++i) {
        add({[this, i] { return fields_[i]->focused(); }, [this, i](bool f) { fields_[i]->set_focused(f); },
             [this, i] { return mode_ == DatePickerMode::input && fields_[i]->can_focus(); }, [this, i] { return fields_[i]->bounds(); }});
    }
}

bool DateRangePicker::focused() const {
    return toggle_.focused() || grid_focus_.focused() || fields_[0]->focused() || fields_[1]->focused();
}

void DateRangePicker::clear_focus() {
    toggle_.set_focused(false);
    grid_focus_.set_focused(false);
    fields_[0]->set_focused(false);
    fields_[1]->set_focused(false);
}

void DateRangePicker::set_enabled(bool enabled) {
    if (!enabled_.set(enabled)) {
        return;
    }
    toggle_.set_enabled(enabled);
    fields_[0]->set_enabled(enabled);
    fields_[1]->set_enabled(enabled);
    if (!enabled) {
        clear_focus();
        cancel_interaction();
    }
}

AccessibleNode DateRangePicker::accessible() const {
    AccessibleNode node{AccessibleRole::group, title(), bounds(), {.enabled = enabled()}};
    const auto verbose = [this](const std::optional<calendar::Date>& d) {
        return d ? calendar::format_date(locale_->long_date_pattern, *d, locale_->names) : locale_->strings.no_selection_description;
    };
    const std::string description =
        calendar::substitute(locale_->strings.labelled_value, locale_->strings.range_start_headline, verbose(start_)) +
        locale_->strings.list_separator +
        calendar::substitute(locale_->strings.labelled_value, locale_->strings.range_end_headline, verbose(end_));
    node.children.push_back({AccessibleRole::status, headline(), {origin_.x, origin_.y, width(), header_height()}, {}, {}, description});
    if (options_.show_mode_toggle) {
        node.children.push_back(toggle_.accessible());
    }
    if (mode_ == DatePickerMode::input) {
        node.children.push_back(fields_[0]->accessible());
        node.children.push_back(fields_[1]->accessible());
        return node;
    }
    for (AccessibleNode& w : weekday_accessible(*locale_, {origin_.x + t.horizontal_padding, content_top()})) {
        node.children.push_back(std::move(w));
    }
    const render::Rect area = list_area();
    const int first = std::max(0, static_cast<int>(list_.offset() / item_height()));
    const int last = std::min(month_count() - 1, static_cast<int>((list_.offset() + area.height) / item_height()));
    const MonthGridState s = grid_state(false);
    for (int i = first; i <= last; ++i) {
        const calendar::YearMonth ym = month_at(i);
        AccessibleNode month{AccessibleRole::group, calendar::format_date(locale_->year_month_pattern, calendar::first_day(ym), locale_->names),
                             {area.x, area.y + i * item_height() - list_.offset(), area.width, item_height()},
                             {.enabled = enabled(), .focusable = enabled(), .focused = grid_focus_.focused() && calendar::year_month(focus_date_) == ym}};
        month_accessible(month.children, grid_origin(i), ym, s);
        node.children.push_back(std::move(month));
    }
    return node;
}

void DateRangePicker::on_input_changed(std::size_t field, DatePickerResponse& response) {
    const std::string digits = fields_[field]->text();
    std::optional<calendar::Date> parsed;
    std::string error;
    if (digits.size() == calendar::input_pattern_digits(locale_->input)) {
        error = validate_date_input(*locale_, digits, options_.year_range, options_.selectable, &parsed);
        if (error.empty()) {
            const bool wrong = field == 0 ? (end_ && *parsed > *end_) : (start_ && *parsed < *start_);
            if (wrong) {
                error = locale_->strings.invalid_range_input;
                parsed.reset();
            }
        }
    }
    errors_[field] = error;
    fields_[field]->set_supporting_text(error);
    fields_[field]->set_error(!error.empty());
    if (field == 0) {
        if (parsed != start_) {
            start_ = parsed;
            response.changed = true;
        }
    } else if (parsed != end_) {
        end_ = parsed;
        response.changed = true;
    }
}

DatePickerResponse DateRangePicker::handle(const input::PointerEvent& e) {
    DatePickerResponse response;
    grid_focus_.release_on_press(e);
    if (!enabled()) {
        return response;
    }
    if (options_.show_mode_toggle && toggle_.handle(e)) {
        set_mode(mode_ == DatePickerMode::picker ? DatePickerMode::input : DatePickerMode::picker);
        response.consumed = true;
        return response;
    }
    if (mode_ == DatePickerMode::input) {
        for (std::size_t i = 0; i < 2; ++i) {
            const std::string before = fields_[i]->text();
            if (fields_[i]->handle(e)) {
                response.consumed = true;
            }
            if (fields_[i]->text() != before) {
                on_input_changed(i, response);
            }
        }
        return response;
    }
    if (list_.handle(e)) {
        response.consumed = true;
        return response;
    }
    std::optional<calendar::Date> over;
    const render::Rect area = list_area();
    if (e.action != input::PointerAction::leave && contains(area, e.x_dp, e.y_dp)) {
        const int index = static_cast<int>((e.y_dp - area.y + list_.offset()) / item_height());
        if (index >= 0 && index < month_count()) {
            over = day_at(grid_origin(index), month_at(index), locale_->first_day_of_week, e.x_dp, e.y_dp);
            if (over && !day_enabled(grid_state(false), *over)) {
                over.reset();
            }
        }
    }
    hovered_ = over;
    switch (e.action) {
    case input::PointerAction::down:
        pressed_ = over;
        break;
    case input::PointerAction::up:
        if (over && over == pressed_) {
            focus_date_ = *over;
            click(*over, response);
        }
        pressed_.reset();
        break;
    case input::PointerAction::leave:
        pressed_.reset();
        break;
    case input::PointerAction::move:
        break;
    }
    state_layer_.animate_to(pressed_ ? tokens::state_layer_opacity::pressed : over ? tokens::state_layer_opacity::hover : 0.0);
    if (contains(area, e.x_dp, e.y_dp)) {
        response.consumed = true;
    }
    return response;
}

DatePickerResponse DateRangePicker::handle(const input::KeyEvent& e) {
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
    for (std::size_t i = 0; i < 2; ++i) {
        if (fields_[i]->focused()) {
            const std::string before = fields_[i]->text();
            fields_[i]->handle(e);
            response.consumed = e.key != input::Key::escape && e.key != input::Key::tab;
            if (fields_[i]->text() != before) {
                on_input_changed(i, response);
            }
            return response;
        }
    }
    if (grid_focus_.focused()) {
        if (const std::optional<calendar::Date> next = grid_key_target(e, focus_date_, locale_->first_day_of_week)) {
            focus_day(*next);
            response.consumed = true;
            return response;
        }
        if (grid_focus_.activates(e, KeyActivation::space_or_enter)) {
            click(focus_date_, response);
            response.consumed = true;
        }
        return response;
    }
    return response;
}

void DateRangePicker::handle(const input::TextEvent& e) {
    for (std::size_t i = 0; i < 2; ++i) {
        if (fields_[i]->focused()) {
            const std::string before = fields_[i]->text();
            fields_[i]->handle(e);
            if (fields_[i]->text() != before) {
                DatePickerResponse ignored;
                on_input_changed(i, ignored);
            }
        }
    }
}

bool DateRangePicker::handle(const input::WheelEvent& e) {
    if (!enabled() || mode_ != DatePickerMode::picker) {
        return false;
    }
    return list_.handle(e);
}

void DateRangePicker::cancel_interaction() {
    toggle_.cancel_interaction();
    fields_[0]->cancel_interaction();
    fields_[1]->cancel_interaction();
    list_.cancel_interaction();
    hovered_.reset();
    pressed_.reset();
    state_layer_.animate_to(0.0);
}

void DateRangePicker::step(double dt) {
    toggle_.step(dt);
    fields_[0]->step(dt);
    fields_[1]->step(dt);
    grid_focus_.step(dt);
    state_layer_.step(dt);
    list_.step(dt);
    mode_alpha_.step(dt);
    selection_fade_.step(dt);
    enabled_.step(dt);
}

bool DateRangePicker::animating() const {
    return toggle_.animating() || fields_[0]->animating() || fields_[1]->animating() || !grid_focus_.settled() || !state_layer_.settled() ||
           list_.animating() || !mode_alpha_.settled() || !selection_fade_.settled() || !enabled_.settled();
}

std::optional<render::Rect> DateRangePicker::caret_area() const {
    for (const auto& f : fields_) {
        if (f->focused()) {
            return f->caret_area();
        }
    }
    return std::nullopt;
}

std::optional<double> DateRangePicker::next_wake_seconds() const {
    std::optional<double> wake;
    for (const auto& f : fields_) {
        if (f->focused()) {
            wake = f->next_blink_seconds();
        }
    }
    const double scroll = list_.next_wake_seconds();
    if (scroll >= 0.0) {
        wake = wake ? std::min(*wake, scroll) : scroll;
    }
    return wake;
}

void DateRangePicker::paint_picker(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    paint_weekdays(canvas, scheme, fonts_, *locale_, {origin_.x + t.horizontal_padding, content_top()});
    const render::Rect area = list_area();
    canvas.push_clip(area);
    const MonthGridState s = grid_state(true);
    const int first = std::max(0, static_cast<int>(list_.offset() / item_height()));
    const int last = std::min(month_count() - 1, static_cast<int>((list_.offset() + area.height) / item_height()));
    const text::TextStyle subhead = style_of(t.subhead_type);
    for (int i = first; i <= last; ++i) {
        const calendar::YearMonth ym = month_at(i);
        const double top = area.y + i * item_height() - list_.offset();
        const text::TextLine line =
            text::layout_line(fonts_, calendar::format_date(locale_->year_month_pattern, calendar::first_day(ym), locale_->names), subhead);
        text::draw_line(canvas, fonts_, line, {origin_.x + t.subhead_start, top + t.subhead_top}, dim(scheme[k.subhead], enabled_.amount()));
        paint_month(canvas, scheme, fonts_, grid_origin(i), ym, s);
    }
    canvas.pop_clip();
    list_.paint(canvas, scheme);
}

void DateRangePicker::paint_inputs(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    fields_[0]->paint(canvas, scheme);
    fields_[1]->paint(canvas, scheme);
}

void DateRangePicker::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const render::Rect b = bounds();
    canvas.fill_rect(b, scheme[k.container]);
    const double e = enabled_.amount();
    const double start = t.range_header_start;
    const text::TextLine title_line = text::layout_line(fonts_, title(), style_of(t.title_type));
    text::draw_line(canvas, fonts_, title_line, {origin_.x + start, origin_.y + t.title_top}, dim(scheme[k.title], e));
    const text::TextLine headline_line = text::layout_line(fonts_, headline(), style_of(t.range_headline_type));
    const double slot = tokens::minimum_interactive_size;
    const double row_top = origin_.y + header_height() - t.divider_thickness - slot - t.toggle_bottom;
    const double headline_y = row_top + (slot + t.toggle_bottom - t.headline_bottom - headline_line.box_height()) / 2.0;
    const double headline_width = t.container_width - start - t.header_end - (options_.show_mode_toggle ? slot : 0.0);
    canvas.push_clip({origin_.x + start, origin_.y, headline_width, header_height()});
    text::draw_line(canvas, fonts_, headline_line, {origin_.x + start, headline_y}, dim(scheme[k.headline], e));
    canvas.pop_clip();
    if (options_.show_mode_toggle) {
        toggle_.paint(canvas, scheme);
    }
    canvas.fill_rect({origin_.x, origin_.y + header_height() - t.divider_thickness, t.container_width, t.divider_thickness}, scheme[k.divider]);
    const double a = std::clamp(mode_alpha_.value(), 0.0, 1.0);
    canvas.push_clip({b.x, content_top(), b.width, b.y + b.height - content_top()});
    const bool picker = mode_ == DatePickerMode::picker;
    if (a < 1.0) {
        const color::ColorScheme out = color::faded(scheme, 1.0 - a);
        if (picker) {
            paint_inputs(canvas, out);
        } else {
            paint_picker(canvas, out);
        }
    }
    const color::ColorScheme in = a < 1.0 ? color::faded(scheme, a) : scheme;
    if (picker) {
        paint_picker(canvas, in);
    } else {
        paint_inputs(canvas, in);
    }
    canvas.pop_clip();
}

}
