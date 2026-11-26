#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/clipboard.hpp"
#include "lamina/input/events.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/style/style.hpp"
#include "lamina/text/font.hpp"
#include "lamina/tokens/search.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/icon_button.hpp"
#include "lamina/widgets/list_view.hpp"
#include "lamina/widgets/pattern_strings.hpp"
#include "lamina/widgets/text_field.hpp"

namespace lamina::widgets {

enum class SearchViewMode { automatic, full_screen, docked };

struct SearchBarIcon {
    icons::Icon icon;
    std::string accessible_name;
};

struct SearchBarOptions {
    std::string placeholder;
    icons::Icon search_icon;
    SearchBarIcon back;
    std::optional<SearchBarIcon> trailing = std::nullopt;
    std::vector<ListItemSpec> results = {};
    SearchViewMode mode = SearchViewMode::automatic;
    double compact_width = current_style().search.compact_width;
    input::Clipboard* clipboard = nullptr;
    const PatternStrings* strings = &english_us_patterns();
    std::string accessible_name = {};
};

struct SearchBarResponse {
    bool expanded = false;
    bool collapsed = false;
    bool query_changed = false;
    std::optional<std::string> searched = std::nullopt;
    std::optional<std::size_t> chosen = std::nullopt;
    bool trailing = false;
    bool consumed = false;
};

class SearchBar {
public:
    SearchBar(SearchBarOptions options, const text::FontStack& fonts);
    SearchBar(const SearchBar&) = delete;
    SearchBar& operator=(const SearchBar&) = delete;

    layout::Size measure(const layout::Constraints& c) const;
    void arrange(const render::Rect& rect);
    render::Rect bounds() const { return collapsed_; }
    void set_area(const render::Rect& area);
    render::Rect area() const { return area_; }
    bool full_screen() const;
    render::Rect surface() const;
    render::Rect field_rect() const;
    render::Rect results_rect() const;
    double corner_radius() const;

    void expand();
    void collapse();
    bool expanded() const { return progress_.target() == 1.0; }
    bool is_open() const;
    bool holds_pointer() const { return holds_press() || results_.holds_pointer(); }
    double progress() const { return std::clamp(progress_.value(), 0.0, 1.0); }
    double raw_progress() const { return progress_.value(); }

    std::string query() const { return field_.text(); }
    void set_query(std::string_view text);
    void set_results(std::vector<ListItemSpec> results);
    TextField& field() { return field_; }
    const TextField& field() const { return field_; }
    ListView& results() { return results_; }
    const ListView& results() const { return results_; }
    IconButton& back_button() { return *back_; }
    IconButton* trailing_button() { return trailing_ ? trailing_.get() : nullptr; }
    void add_focus_stops(FocusManager& focus);

    bool focused() const;
    void set_focused(bool focused);
    bool enabled() const { return field_.enabled(); }
    void set_enabled(bool enabled);
    bool can_focus() const { return field_.can_focus(); }
    std::optional<render::Rect> caret_area() const;
    std::optional<double> next_wake_seconds() const;

    SearchBarResponse handle(const input::PointerEvent& event);
    SearchBarResponse handle(const input::KeyEvent& event);
    SearchBarResponse handle(const input::TextEvent& event);
    bool handle(const input::WheelEvent& event);
    void cancel_interaction();
    void restyle();
    void step(double dt_seconds);
    bool animating() const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    void paint_expanded(render::Canvas& canvas, const color::ColorScheme& scheme) const;
    std::optional<AccessibleNode> accessible() const;

private:
    enum class PointerPart { none, back, trailing, field, results, outside };

    bool showing_view() const;
    bool holds_press() const { return pressed_ != PointerPart::none && pressed_ != PointerPart::outside; }
    PointerPart part_at(const input::PointerEvent& event) const;
    void deliver(PointerPart part, const input::PointerEvent& event, SearchBarResponse& r);
    void close_from_pointer();
    void place();
    void paint_field(render::Canvas& canvas, const color::ColorScheme& scheme, const render::Rect& field, bool expanded_view) const;
    void tab(bool backward);
    void focus_results();
    SearchBarResponse changed(const std::string& before, SearchBarResponse r);

    SearchBarOptions options_;
    text::FontStack fonts_;
    TextField field_;
    std::unique_ptr<IconButton> back_;
    std::unique_ptr<IconButton> trailing_;
    ListView results_;
    FocusManager top_focus_;
    FocusManager list_focus_;
    motion::Animated progress_;
    render::Rect collapsed_{0.0, 0.0, 0.0, 0.0};
    render::Rect area_{0.0, 0.0, 0.0, 0.0};
    bool field_pressed_ = false;
    bool claim_focus_ = false;
    PointerPart pressed_ = PointerPart::none;
    std::optional<render::Rect> arranged_;
    StyleStamp style_stamp_;
};

}
