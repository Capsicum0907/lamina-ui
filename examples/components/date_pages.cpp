#include <array>
#include <chrono>
#include <ctime>
#include <memory>
#include <string>

#include "components/base_page.hpp"
#include "components/pages.hpp"
#include "lamina/calendar/locale.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/date_picker.hpp"
#include "lamina/widgets/date_range_picker.hpp"
#include "lamina/widgets/docked_date_picker.hpp"
#include "lamina/widgets/picker_dialog.hpp"
#include "lamina/widgets/segmented_button.hpp"
#include "res.hpp"
#include "lamina/style/style.hpp"

namespace lamina::examples::components {

namespace {

struct DatesLayout {
    double column_space = 40.0;
    double row_gap = 16.0;
    double range_height = 560.0;
    std::array<const char*, 3> locale_names{"English (US)", "English (UK)", "日本語"};
};

const DatesLayout dates_layout;

const calendar::CalendarLocale& locale_at(std::size_t i) {
    switch (i) {
    case 1: return calendar::english_gb();
    case 2: return calendar::japanese();
    default: return calendar::english_us();
    }
}

bool weekday_only(const calendar::Date& d) {
    const calendar::Weekday w = calendar::weekday(d);
    return w != calendar::Weekday::saturday && w != calendar::Weekday::sunday;
}

class DatesPage final : public BasePage {
public:
    explicit DatesPage(PageContext context, calendar::Date today) : BasePage(std::move(context)), today_(today) {
        const DatesLayout& l = dates_layout;
        widgets::SegmentedButtonRowOptions lo{{}, widgets::SegmentSelection::single, "Locale"};
        for (std::size_t i = 0; i < l.locale_names.size(); ++i) {
            lo.segments.push_back({l.locale_names[i], std::nullopt, i == 0});
        }
        locale_row_ = &controls.emplace_with<widgets::SegmentedButtonRow>(
            [this](widgets::SegmentedButtonRow& r, const widgets::SegmentedButtonResponse& response) {
                if (response.clicked) {
                    set_locale(locale_at(*r.selected()));
                }
            },
            std::move(lo), ctx.fonts, ctx.icon_font);
        focus_.add(*locale_row_);

        calendar::Date first_weekday = today_;
        while (!weekday_only(first_weekday)) {
            first_weekday = calendar::add_days(first_weekday, 1);
        }
        widgets::DatePickerOptions inline_options{.today = today_, .selected = first_weekday};
        inline_options.selectable.date = weekday_only;
        inline_ = &controls.emplace_with<widgets::DatePicker>(
            [this](widgets::DatePicker& p, const widgets::DatePickerResponse& r) {
                if (r.changed) {
                    report_date("Date picker", p.selected(), p.locale());
                }
            },
            std::move(inline_options), ctx.fonts, ctx.icon_font);
        inline_->add_focus_stops(focus_);

        range_ = &controls.emplace_with<widgets::DateRangePicker>(
            [this](widgets::DateRangePicker& p, const widgets::DatePickerResponse& r) {
                if (r.changed) {
                    report_range(p);
                }
            },
            widgets::DateRangePickerOptions{.today = today_, .height = l.range_height}, ctx.fonts, ctx.icon_font);
        range_->add_focus_stops(focus_);

        docked_ = &controls.emplace_with<widgets::DockedDatePicker>(
            [this](widgets::DockedDatePicker& d, const widgets::DatePickerResponse& r) {
                if (r.changed) {
                    report_date("Docked", d.selected(), inline_->locale());
                }
            },
            widgets::DockedDatePickerOptions{.today = today_, .open_name = "Open calendar"}, ctx.fonts, ctx.icon_font);
        docked_->add_focus_stops(focus_);

        dialog_picker_ = std::make_unique<widgets::DatePicker>(widgets::DatePickerOptions{.today = today_}, ctx.fonts, ctx.icon_font);
        dialog_ = std::make_unique<widgets::PickerDialog>(
            widgets::PickerDialogOptions{"OK", "Cancel", widgets::PickerDialogLayout::date, current_style().date_picker.dialog_corner, {},
                                         std::nullopt, "Date picker dialog"},
            std::make_unique<widgets::PaneOf<widgets::DatePicker>>(*dialog_picker_), ctx.fonts);
        dialog_range_ = std::make_unique<widgets::DateRangePicker>(widgets::DateRangePickerOptions{.today = today_}, ctx.fonts, ctx.icon_font);
        range_dialog_ = std::make_unique<widgets::PickerDialog>(
            widgets::PickerDialogOptions{"OK", "Cancel", widgets::PickerDialogLayout::date, current_style().date_picker.dialog_corner, {},
                                         std::nullopt, "Date range dialog"},
            std::make_unique<widgets::PaneOf<widgets::DateRangePicker>>(*dialog_range_), ctx.fonts);

        open_date_ = &controls.emplace_with<widgets::Button>(
            [this](widgets::Button&, bool clicked) {
                if (clicked) {
                    dialog_->open_in(window_rect(), source_);
                }
            },
            widgets::ButtonOptions{"Date picker dialog", tokens::ButtonStyle::tonal, tokens::ButtonSize::small, tokens::ButtonShape::round,
                                   icon(res::icons::event)},
            ctx.fonts);
        focus_.add(*open_date_);
        open_range_ = &controls.emplace_with<widgets::Button>(
            [this](widgets::Button&, bool clicked) {
                if (clicked) {
                    range_dialog_->open_in(window_rect(), source_);
                }
            },
            widgets::ButtonOptions{"Date range dialog", tokens::ButtonStyle::tonal, tokens::ButtonSize::small, tokens::ButtonShape::round,
                                   icon(res::icons::date_range)},
            ctx.fonts);
        focus_.add(*open_range_);

        gated_tip(inline_->mode_toggle(), [this] { return inline_->header_shown(); });
        gated_tip(inline_->previous_button(), [this] { return inline_->navigation_shown(); });
        gated_tip(inline_->next_button(), [this] { return inline_->navigation_shown(); });
        gated_tip(range_->mode_toggle(), [] { return true; });
        gated_tip(docked_->open_button(), [] { return true; });
        gated_tip(docked_->picker().previous_button(), [this] { return docked_->is_open() && docked_->picker().navigation_shown(); });
        gated_tip(docked_->picker().next_button(), [this] { return docked_->is_open() && docked_->picker().navigation_shown(); });
        gated_tip(dialog_picker_->mode_toggle(), [this] { return dialog_->is_open(); }, true);
        gated_tip(dialog_picker_->previous_button(), [this] { return dialog_->is_open() && dialog_picker_->navigation_shown(); }, true);
        gated_tip(dialog_picker_->next_button(), [this] { return dialog_->is_open() && dialog_picker_->navigation_shown(); }, true);
        gated_tip(dialog_range_->mode_toggle(), [this] { return range_dialog_->is_open(); }, true);

        auto buttons = layout::column({widgets::node(*open_date_), widgets::node(*open_range_)});
        buttons->spacing(l.row_gap);
        auto pickers = columns({{heading("Inline (weekdays only)", widgets::node(*inline_))},
                                {heading("Range", widgets::node(*range_))},
                                {heading("Docked", widgets::node(*docked_)), heading("Dialogs", buttons)}},
                               l.column_space, l.row_gap * 2.0);
        root = columns({{captioned("Locale", widgets::node(*locale_row_)), pickers}}, l.column_space, l.row_gap);
    }

    bool holds_pointer() const override {
        return dialog_->is_open() || range_dialog_->is_open() || BasePage::holds_pointer();
    }
    bool handle(const input::PointerEvent& e) override {
        source_ = input::InputSource::pointer;
        if (auto* d = open_dialog()) {
            handle_dialog_tips(e);
            finish(*d, d->handle(e));
            return true;
        }
        return BasePage::handle(e);
    }
    bool handle(const input::KeyEvent& e) override {
        source_ = input::InputSource::keyboard;
        if (auto* d = open_dialog()) {
            handle_dialog_tips(e);
            finish(*d, d->handle(e));
            return true;
        }
        return BasePage::handle(e);
    }
    void handle(const input::TextEvent& e) override {
        if (auto* d = open_dialog()) {
            d->handle(e);
            return;
        }
        controls.handle(e);
    }
    bool handle(const input::WheelEvent& e) override {
        if (auto* d = open_dialog()) {
            return d->handle(e);
        }
        return inline_->handle(e) || range_->handle(e) || docked_->handle(e);
    }
    void step(double dt) override {
        BasePage::step(dt);
        dialog_->step(dt);
        range_dialog_->step(dt);
    }
    bool animating() const override { return BasePage::animating() || dialog_->animating() || range_dialog_->animating(); }
    void paint_overlays(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        docked_->paint_popup(canvas, scheme);
        BasePage::paint_overlays(canvas, scheme);
        dialog_->paint(canvas, scheme);
        range_dialog_->paint(canvas, scheme);
        paint_dialog_tips(canvas, scheme);
    }
    std::vector<widgets::AccessibleNode> accessible_nodes() const override {
        std::vector<widgets::AccessibleNode> nodes = BasePage::accessible_nodes();
        for (const auto* d : {dialog_.get(), range_dialog_.get()}) {
            if (auto n = d->accessible()) {
                nodes.push_back(std::move(*n));
            }
        }
        return nodes;
    }
    void cancel_interaction() override {
        BasePage::cancel_interaction();
        dialog_->close();
        range_dialog_->close();
        docked_->close();
    }
    std::optional<render::Rect> caret_area() const override {
        for (const auto* d : {dialog_.get(), range_dialog_.get()}) {
            if (const auto c = d->caret_area()) {
                return c;
            }
        }
        if (const auto c = inline_->caret_area()) {
            return c;
        }
        if (const auto c = range_->caret_area()) {
            return c;
        }
        return docked_->caret_area();
    }
    std::optional<double> next_wake_seconds() const override {
        std::optional<double> wake = BasePage::next_wake_seconds();
        auto merge = [&wake](std::optional<double> w) {
            if (w) {
                wake = wake ? std::min(*wake, *w) : *w;
            }
        };
        merge(inline_->next_wake_seconds());
        merge(range_->next_wake_seconds());
        merge(docked_->next_wake_seconds());
        merge(dialog_->next_wake_seconds());
        merge(range_dialog_->next_wake_seconds());
        return wake;
    }
    void set_window(layout::Size window) override {
        BasePage::set_window(window);
        window_ = window;
        dialog_->set_area(window_rect());
        range_dialog_->set_area(window_rect());
    }

private:
    render::Rect window_rect() const { return {0.0, 0.0, window_.width, window_.height}; }

    widgets::PickerDialog* open_dialog() {
        if (dialog_->is_open()) {
            return dialog_.get();
        }
        if (range_dialog_->is_open()) {
            return range_dialog_.get();
        }
        return nullptr;
    }

    void finish(widgets::PickerDialog& d, widgets::DialogResult result) {
        if (result == widgets::DialogResult::confirmed) {
            if (&d == dialog_.get()) {
                report_date("Dialog", dialog_picker_->selected(), dialog_picker_->locale());
            } else {
                report_range(*dialog_range_);
            }
        } else if (result == widgets::DialogResult::dismissed) {
            ctx.report("Dialog dismissed");
        }
        if (result != widgets::DialogResult::none && source_ == input::InputSource::keyboard) {
            (&d == dialog_.get() ? open_date_ : open_range_)->set_focused(true);
        }
    }

    void report_date(const char* what, std::optional<calendar::Date> d, const calendar::CalendarLocale& l) {
        ctx.report(std::string(what) + ": " + (d ? calendar::format_date(l.short_date_pattern, *d, l.names) : std::string("none")));
    }

    void report_range(const widgets::DateRangePicker& p) { ctx.report("Range: " + p.headline()); }

    void set_locale(const calendar::CalendarLocale& l) {
        inline_->set_locale(l);
        range_->set_locale(l);
        docked_->set_locale(l);
        dialog_picker_->set_locale(l);
        dialog_range_->set_locale(l);
        dialog_->set_labels(l.strings.confirm, l.strings.dismiss);
        range_dialog_->set_labels(l.strings.confirm, l.strings.dismiss);
        ctx.report("Locale: " + l.tag);
    }

    calendar::Date today_;
    layout::Size window_{0.0, 0.0};
    input::InputSource source_ = input::InputSource::pointer;
    widgets::SegmentedButtonRow* locale_row_ = nullptr;
    widgets::DatePicker* inline_ = nullptr;
    widgets::DateRangePicker* range_ = nullptr;
    widgets::DockedDatePicker* docked_ = nullptr;
    widgets::Button* open_date_ = nullptr;
    widgets::Button* open_range_ = nullptr;
    std::unique_ptr<widgets::DatePicker> dialog_picker_;
    std::unique_ptr<widgets::PickerDialog> dialog_;
    std::unique_ptr<widgets::DateRangePicker> dialog_range_;
    std::unique_ptr<widgets::PickerDialog> range_dialog_;
};

calendar::Date local_today() {
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
    const auto now = std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::system_clock::now()}.get_local_time();
    const std::chrono::year_month_day ymd{std::chrono::floor<std::chrono::days>(now)};
    return {static_cast<int>(ymd.year()), static_cast<int>(static_cast<unsigned>(ymd.month())), static_cast<int>(static_cast<unsigned>(ymd.day()))};
#else
    constexpr int tm_year_base = 1900;
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    return {local.tm_year + tm_year_base, local.tm_mon + 1, local.tm_mday};
#endif
}

}

std::unique_ptr<Page> make_dates_page(PageContext context) {
    return std::make_unique<DatesPage>(std::move(context), local_today());
}

}
