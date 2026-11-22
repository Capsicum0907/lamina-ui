#include <array>
#include <memory>
#include <string>

#include "components/base_page.hpp"
#include "components/pages.hpp"
#include "components/side_scroll.hpp"
#include "lamina/calendar/locale.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/picker_dialog.hpp"
#include "lamina/widgets/segmented_button.hpp"
#include "lamina/widgets/time_picker.hpp"
#include "res.hpp"

namespace lamina::examples::components {

namespace {

struct TimesLayout {
    double column_space = 48.0;
    double row_gap = 16.0;
    int hour = 9;
    int minute = 41;
    std::array<const char*, 3> locale_names{"English (US)", "English (UK)", "日本語"};
    std::array<const char*, 2> clock_names{"12-hour", "24-hour"};
    const char* horizontal_name = "Horizontal";
    const char* side_scroll_name = "Horizontal time picker";
};

const TimesLayout times_layout;

const calendar::CalendarLocale& time_locale_at(std::size_t i) {
    switch (i) {
    case 1: return calendar::english_gb();
    case 2: return calendar::japanese();
    default: return calendar::english_us();
    }
}

class TimesPage final : public BasePage {
public:
    explicit TimesPage(PageContext context) : BasePage(std::move(context)) {
        const TimesLayout& l = times_layout;
        widgets::SegmentedButtonRowOptions lo{{}, widgets::SegmentSelection::single, "Time locale"};
        for (std::size_t i = 0; i < l.locale_names.size(); ++i) {
            lo.segments.push_back({l.locale_names[i], std::nullopt, i == 0});
        }
        auto& locales = controls.emplace_with<widgets::SegmentedButtonRow>(
            [this](widgets::SegmentedButtonRow& r, const widgets::SegmentedButtonResponse& response) {
                if (response.clicked) {
                    set_locale(time_locale_at(*r.selected()));
                }
            },
            std::move(lo), ctx.fonts, ctx.icon_font);
        focus_.add(locales);
        widgets::SegmentedButtonRowOptions co{{}, widgets::SegmentSelection::single, "Clock"};
        for (std::size_t i = 0; i < l.clock_names.size(); ++i) {
            co.segments.push_back({l.clock_names[i], std::nullopt, i == 0});
        }
        clock_ = &controls.emplace_with<widgets::SegmentedButtonRow>(
            [this](widgets::SegmentedButtonRow& r, const widgets::SegmentedButtonResponse& response) {
                if (response.clicked) {
                    set_24_hour(*r.selected() == 1);
                }
            },
            std::move(co), ctx.fonts, ctx.icon_font);
        focus_.add(*clock_);

        auto picker = [this, &l](widgets::TimePickerLayout layout, widgets::TimePickerMode mode, const char* name) -> widgets::TimePicker& {
            auto& p = controls.emplace_with<widgets::TimePicker>(
                [this, name](widgets::TimePicker& w, const widgets::TimePickerResponse& r) {
                    if (r.changed) {
                        ctx.report(std::string(name) + ": " + w.formatted());
                    }
                },
                widgets::TimePickerOptions{.hour = l.hour, .minute = l.minute, .is_24_hour = false, .layout = layout, .mode = mode}, ctx.fonts);
            p.add_focus_stops(focus_);
            pickers_.push_back(&p);
            return p;
        };
        auto& dial = picker(widgets::TimePickerLayout::vertical, widgets::TimePickerMode::dial, "Dial");
        wide_ = std::make_unique<widgets::TimePicker>(
            widgets::TimePickerOptions{.hour = l.hour, .minute = l.minute, .is_24_hour = false, .layout = widgets::TimePickerLayout::horizontal},
            ctx.fonts);
        wide_->add_focus_stops(focus_);
        pickers_.push_back(wide_.get());
        side_ = &controls.emplace_with<SideScroll<widgets::TimePicker>>(
            [this](SideScroll<widgets::TimePicker>&, const widgets::TimePickerResponse& r) {
                if (r.changed) {
                    ctx.report(std::string(times_layout.horizontal_name) + ": " + wide_->formatted());
                }
            },
            *wide_, std::string(l.side_scroll_name));
        auto& typed = picker(widgets::TimePickerLayout::vertical, widgets::TimePickerMode::input, "Input");

        dialog_picker_ = std::make_unique<widgets::TimePicker>(
            widgets::TimePickerOptions{.hour = l.hour, .minute = l.minute, .is_24_hour = false}, ctx.fonts);
        dialog_ = std::make_unique<widgets::PickerDialog>(
            widgets::PickerDialogOptions{"OK", "Cancel", widgets::PickerDialogLayout::time, tokens::CornerSize::extra_large,
                                         locale_->strings.time_picker_title,
                                         widgets::PickerDialogLeading{{ctx.icon_font, tokens::time_picker_tokens.keyboard_symbol},
                                                                      locale_->strings.switch_to_text_input,
                                                                      [this](widgets::IconButton& b) { toggle_dialog_mode(b); }},
                                         "Time picker dialog"},
            std::make_unique<widgets::PaneOf<widgets::TimePicker>>(*dialog_picker_), ctx.fonts);
        open_ = &controls.emplace_with<widgets::Button>(
            [this](widgets::Button&, bool clicked) {
                if (clicked) {
                    dialog_->open_in(window_rect(), source_);
                }
            },
            widgets::ButtonOptions{"Time picker dialog", tokens::ButtonStyle::tonal, tokens::ButtonSize::small, tokens::ButtonShape::round,
                                   icon(res::icons::schedule)},
            ctx.fonts);
        focus_.add(*open_);

        gated_tip(*dialog_->leading(), [this] { return dialog_->is_open(); }, true);
        auto pickers = columns({{heading("Dial", widgets::node(dial))},
                                {heading(l.horizontal_name, widgets::node(*side_)), heading("Input", widgets::node(typed)), heading("Dialog", widgets::node(*open_))}},
                               l.column_space, l.row_gap * 2.0);
        auto settings = items({captioned("Locale", widgets::node(locales)), captioned("Clock", widgets::node(*clock_))});
        settings->spacing(l.column_space).line_spacing(l.row_gap);
        root = columns({{settings, pickers}}, l.column_space, l.row_gap * 1.5);
    }

    bool handle(const input::PointerEvent& e) override {
        source_ = input::InputSource::pointer;
        if (dialog_->is_open()) {
            handle_dialog_tips(e);
            finish(dialog_->handle(e));
            return true;
        }
        return BasePage::handle(e);
    }
    bool handle(const input::WheelEvent& e) override { return side_->handle(e) || BasePage::handle(e); }
    bool handle(const input::KeyEvent& e) override {
        source_ = input::InputSource::keyboard;
        if (dialog_->is_open()) {
            handle_dialog_tips(e);
            finish(dialog_->handle(e));
            return true;
        }
        return BasePage::handle(e);
    }
    void handle(const input::TextEvent& e) override {
        if (dialog_->is_open()) {
            dialog_->handle(e);
            return;
        }
        controls.handle(e);
    }
    void step(double dt) override {
        BasePage::step(dt);
        dialog_->step(dt);
    }
    bool animating() const override { return BasePage::animating() || dialog_->animating(); }
    void paint_overlays(render::Canvas& canvas, const color::ColorScheme& scheme) const override {
        BasePage::paint_overlays(canvas, scheme);
        dialog_->paint(canvas, scheme);
        paint_dialog_tips(canvas, scheme);
    }
    std::vector<widgets::AccessibleNode> accessible_nodes() const override {
        std::vector<widgets::AccessibleNode> nodes = BasePage::accessible_nodes();
        if (auto n = dialog_->accessible()) {
            nodes.push_back(std::move(*n));
        }
        return nodes;
    }
    void cancel_interaction() override {
        BasePage::cancel_interaction();
        dialog_->close();
    }
    std::optional<render::Rect> caret_area() const override {
        if (const auto c = dialog_->caret_area()) {
            return c;
        }
        for (const auto* p : pickers_) {
            if (const auto c = p->caret_area()) {
                return c;
            }
        }
        return std::nullopt;
    }
    std::optional<double> next_wake_seconds() const override {
        std::optional<double> wake = BasePage::next_wake_seconds();
        auto merge = [&wake](std::optional<double> w) {
            if (w) {
                wake = wake ? std::min(*wake, *w) : *w;
            }
        };
        for (const auto* p : pickers_) {
            merge(p->next_wake_seconds());
        }
        merge(dialog_->next_wake_seconds());
        return wake;
    }
    void set_window(layout::Size window) override {
        BasePage::set_window(window);
        window_ = window;
        dialog_->set_area(window_rect());
    }

private:
    render::Rect window_rect() const { return {0.0, 0.0, window_.width, window_.height}; }

    void toggle_dialog_mode(widgets::IconButton& b) {
        const bool to_input = dialog_picker_->mode() == widgets::TimePickerMode::dial;
        dialog_picker_->set_mode(to_input ? widgets::TimePickerMode::input : widgets::TimePickerMode::dial);
        b.set_icon({ctx.icon_font, to_input ? tokens::time_picker_tokens.clock_symbol : tokens::time_picker_tokens.keyboard_symbol});
        b.set_accessible_name(to_input ? locale_->strings.switch_to_clock_mode : locale_->strings.switch_to_text_input);
        dialog_->set_title(to_input ? locale_->strings.time_input_title : locale_->strings.time_picker_title);
    }

    void finish(widgets::DialogResult result) {
        if (result == widgets::DialogResult::confirmed) {
            ctx.report("Dialog: " + dialog_picker_->formatted());
        } else if (result == widgets::DialogResult::dismissed) {
            ctx.report("Dialog dismissed");
        }
        if (result != widgets::DialogResult::none && source_ == input::InputSource::keyboard) {
            open_->set_focused(true);
        }
    }

    void set_locale(const calendar::CalendarLocale& l) {
        locale_ = &l;
        for (auto* p : pickers_) {
            p->set_locale(l);
            p->set_24_hour(l.uses_24_hour_clock);
        }
        dialog_picker_->set_locale(l);
        dialog_picker_->set_24_hour(l.uses_24_hour_clock);
        clock_->set_checked(l.uses_24_hour_clock ? 1 : 0, true);
        dialog_->set_labels(l.strings.confirm, l.strings.dismiss);
        const bool input = dialog_picker_->mode() == widgets::TimePickerMode::input;
        dialog_->set_title(input ? l.strings.time_input_title : l.strings.time_picker_title);
        ctx.report("Locale: " + l.tag);
    }

    void set_24_hour(bool on) {
        for (auto* p : pickers_) {
            p->set_24_hour(on);
        }
        dialog_picker_->set_24_hour(on);
        ctx.report(on ? "Clock: 24-hour" : "Clock: 12-hour");
    }

    const calendar::CalendarLocale* locale_ = &calendar::english_us();
    layout::Size window_{0.0, 0.0};
    input::InputSource source_ = input::InputSource::pointer;
    widgets::SegmentedButtonRow* clock_ = nullptr;
    std::vector<widgets::TimePicker*> pickers_;
    std::unique_ptr<widgets::TimePicker> wide_;
    SideScroll<widgets::TimePicker>* side_ = nullptr;
    widgets::Button* open_ = nullptr;
    std::unique_ptr<widgets::TimePicker> dialog_picker_;
    std::unique_ptr<widgets::PickerDialog> dialog_;
};

}

std::unique_ptr<Page> make_times_page(PageContext context) {
    return std::make_unique<TimesPage>(std::move(context));
}

}
