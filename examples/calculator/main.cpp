#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <memory>
#include <optional>
#include <vector>

#include "calculator/display.hpp"
#include "calculator/engine.hpp"
#include "common/canvas.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_sdl3/app.hpp"
#include "lamina_sdl3/resources.hpp"
#include "lamina_sdl3/system_fonts.hpp"

using namespace lamina;
using examples::calculator::Command;
using input::Key;
using tokens::ButtonShape;
using tokens::ButtonStyle;

namespace {

enum class KeyRole { digit, operation, clear, equals };

struct KeySpec {
    const char* label;
    std::optional<icons::Symbol> icon;
    Command command;
    KeyRole role;
};

struct KeyBinding {
    Key key;
    Command command;
};

constexpr std::size_t keypad_columns = 4;
constexpr std::size_t keypad_rows = 5;

struct Config {
    sdl3::WindowOptions window{"lamina-ui calculator", 400, 780};
    color::SchemeOptions scheme{.seed = color::default_scheme_options.seed, .dark = true, .variant = color::Variant::tonal_spot, .contrast = 0.0};
    color::Role background = color::Role::surface;
    double margin = 8.0;
    double key_gap = 8.0;
    double display_min_height = 160.0;
    tokens::ButtonSize key_size = tokens::ButtonSize::extra_large;
    examples::calculator::DisplayStyle display;
    std::array<widgets::ButtonColors, 4> roles{{
        {color::Role::surface_container_high, color::Role::on_surface},
        {color::Role::secondary_container, color::Role::on_secondary_container},
        {color::Role::tertiary_container, color::Role::on_tertiary_container},
        {color::Role::primary, color::Role::on_primary},
    }};
    std::array<std::array<KeySpec, keypad_columns>, keypad_rows> keypad{{
        {{{"AC", std::nullopt, Command::all_clear, KeyRole::clear},
          {"( )", std::nullopt, Command::parentheses, KeyRole::operation},
          {"%", std::nullopt, Command::percent, KeyRole::operation},
          {"÷", std::nullopt, Command::divide, KeyRole::operation}}},
        {{{"7", std::nullopt, Command::digit_7, KeyRole::digit},
          {"8", std::nullopt, Command::digit_8, KeyRole::digit},
          {"9", std::nullopt, Command::digit_9, KeyRole::digit},
          {"×", std::nullopt, Command::multiply, KeyRole::operation}}},
        {{{"4", std::nullopt, Command::digit_4, KeyRole::digit},
          {"5", std::nullopt, Command::digit_5, KeyRole::digit},
          {"6", std::nullopt, Command::digit_6, KeyRole::digit},
          {"−", std::nullopt, Command::subtract, KeyRole::operation}}},
        {{{"1", std::nullopt, Command::digit_1, KeyRole::digit},
          {"2", std::nullopt, Command::digit_2, KeyRole::digit},
          {"3", std::nullopt, Command::digit_3, KeyRole::digit},
          {"+", std::nullopt, Command::add, KeyRole::operation}}},
        {{{"0", std::nullopt, Command::digit_0, KeyRole::digit},
          {".", std::nullopt, Command::point, KeyRole::digit},
          {"", icons::Symbol::backspace, Command::backspace, KeyRole::digit},
          {"=", std::nullopt, Command::equals, KeyRole::equals}}},
    }};
    std::array<KeyBinding, 23> bindings{{
        {Key::digit_0, Command::digit_0},   {Key::digit_1, Command::digit_1},
        {Key::digit_2, Command::digit_2},   {Key::digit_3, Command::digit_3},
        {Key::digit_4, Command::digit_4},   {Key::digit_5, Command::digit_5},
        {Key::digit_6, Command::digit_6},   {Key::digit_7, Command::digit_7},
        {Key::digit_8, Command::digit_8},   {Key::digit_9, Command::digit_9},
        {Key::period, Command::point},      {Key::plus, Command::add},
        {Key::minus, Command::subtract},    {Key::asterisk, Command::multiply},
        {Key::slash, Command::divide},      {Key::equals, Command::equals},
        {Key::enter, Command::equals},      {Key::percent, Command::percent},
        {Key::paren_left, Command::open_parenthesis}, {Key::paren_right, Command::close_parenthesis},
        {Key::backspace, Command::backspace}, {Key::escape, Command::clear},
        {Key::delete_forward, Command::all_clear},
    }};
};

double key_diameter(const Config& config, double width, double height) {
    const auto columns = static_cast<double>(keypad_columns);
    const auto rows = static_cast<double>(keypad_rows);
    const double by_width = (width - (columns - 1.0) * config.key_gap) / columns;
    const double by_height = (height - config.display_min_height - (rows - 1.0) * config.key_gap) / rows;
    return std::max(0.0, std::min(by_width, by_height));
}

std::optional<Command> command_for(const Config& config, const input::KeyEvent& e) {
    if (e.ctrl) {
        return std::nullopt;
    }
    for (const KeyBinding& b : config.bindings) {
        if (b.key == e.key) {
            return b.command;
        }
    }
    return std::nullopt;
}

struct KeyButton {
    widgets::Button* button;
    Command command;
};

}

int main(int argc, char** argv) {
    try {
        const Config config;
        const text::FontStack fonts = sdl3::font_stack_from_arguments(argc, argv);
        const icons::IconFont icon_font = sdl3::bundled_icon_font();
        const color::ColorScheme scheme = color::spec_2025().build(config.scheme);

        examples::calculator::Engine engine;
        examples::calculator::Display display(config.display, fonts);
        auto refresh = [&] {
            display.show(engine.formula(), engine.result(), engine.focus() == examples::calculator::Focus::result,
                         engine.error() != examples::calculator::Error::none);
        };
        refresh();

        widgets::WidgetSet widgets;
        widgets.add(display);
        std::vector<KeyButton> keys;
        std::vector<layout::NodePtr> rows;
        for (const auto& row_specs : config.keypad) {
            std::vector<layout::NodePtr> cells;
            for (const KeySpec& spec : row_specs) {
                std::optional<icons::Icon> icon;
                if (spec.icon) {
                    icon = icons::Icon{icon_font, *spec.icon};
                }
                auto& button = widgets.emplace<widgets::Button>(
                    widgets::ButtonOptions{spec.label, ButtonStyle::tonal, config.key_size, ButtonShape::round, icon,
                                           config.roles[static_cast<std::size_t>(spec.role)]},
                    fonts);
                keys.push_back({&button, spec.command});
                cells.push_back(layout::weighted(widgets::filled_node(button), 1.0));
            }
            auto row = layout::row(std::move(cells));
            row->spacing(config.key_gap).cross_align(layout::CrossAlign::stretch).fill_cross(true);
            row->self_align = layout::CrossAlign::stretch;
            rows.push_back(layout::weighted(row, 1.0));
        }
        auto grid = layout::column(std::move(rows));
        grid->spacing(config.key_gap).cross_align(layout::CrossAlign::stretch).fill_cross(true);
        double keypad_width = 0.0;
        auto keypad = std::make_shared<layout::Leaf>(
            [&config, &keypad_width](const layout::Constraints& c) {
                const double d = key_diameter(config, c.max_width, c.max_height);
                const auto columns = static_cast<double>(keypad_columns);
                const auto rows = static_cast<double>(keypad_rows);
                keypad_width = columns * d + (columns - 1.0) * config.key_gap;
                return layout::Size{keypad_width, rows * d + (rows - 1.0) * config.key_gap};
            },
            [grid](const render::Rect& r) { layout::run(*grid, r); });
        keypad->self_align = layout::CrossAlign::center;
        auto display_node = std::make_shared<layout::Leaf>(
            [&keypad_width](const layout::Constraints& c) { return layout::Size{keypad_width, c.max_height}; },
            [&display](const render::Rect& r) { display.arrange(r); });
        auto page = layout::column({layout::weighted(display_node, 1.0), keypad});
        page->spacing(config.key_gap).padding(layout::Insets::all(config.margin)).cross_align(layout::CrossAlign::center).fill_cross(true);

        auto update = [&](const sdl3::Frame& frame) {
            layout::run(*page, {0.0, 0.0, frame.width_dp, frame.height_dp});
            bool changed = false;
            for (const auto& e : frame.pointer) {
                for (const KeyButton& k : keys) {
                    if (k.button->handle(e)) {
                        engine.apply(k.command);
                        changed = true;
                    }
                }
            }
            for (const auto& e : frame.keys) {
                if (const auto command = command_for(config, e)) {
                    engine.apply(*command);
                    changed = true;
                }
            }
            if (changed) {
                refresh();
            }
            return scheme[config.background];
        };

        auto paint = [&](render::Canvas& canvas, const sdl3::Frame& frame) {
            widgets.step(frame.dt_seconds);
            widgets.paint(canvas, scheme);
            return widgets.animating();
        };

        sdl3::run_app(config.window, examples::window_canvas(), update, paint);
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
