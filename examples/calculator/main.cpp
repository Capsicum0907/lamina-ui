#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "calculator/display.hpp"
#include "calculator/engine.hpp"
#include "calculator/keypad_layout.hpp"
#include "common/animated_box.hpp"
#include "common/canvas.hpp"
#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina_sdl3/app.hpp"
#include "lamina_sdl3/resources.hpp"
#include "lamina_sdl3/system_fonts.hpp"

using namespace lamina;
using examples::AnimatedBox;
using examples::calculator::AngleUnit;
using examples::calculator::Command;
using examples::calculator::KeyPlacement;
using input::Key;
using tokens::ButtonShape;
using tokens::ButtonStyle;

namespace {

enum class KeyRole { digit, operation, clear, equals };

enum class LabelSwitch { none, inverse, angle_unit };

struct KeySpec {
    const char* label;
    std::optional<icons::Symbol> icon;
    Command command;
    KeyRole role;
    KeyPlacement placement;
    LabelSwitch label_switch = LabelSwitch::none;
    const char* alternate = nullptr;
};

struct KeyBinding {
    Key key;
    Command command;
};

constexpr std::size_t key_count = 32;

struct Config {
    sdl3::WindowOptions window{"lamina-ui calculator", 400, 780};
    color::SchemeOptions scheme{.seed = color::default_scheme_options.seed, .dark = true, .variant = color::Variant::tonal_spot, .contrast = 0.0};
    color::Role background = color::Role::surface;
    examples::calculator::LayoutSettings layout;
    tokens::ButtonSize key_size = tokens::ButtonSize::extra_large;
    double label_ratio = 0.4;
    double icon_ratio = 0.36;
    double key_press_seconds = 0.12;
    motion::SpringSpec move_spring = tokens::motion_scheme.default_spatial();
    motion::SpringSpec fade_spring = tokens::motion_scheme.default_effects();
    examples::calculator::DisplayStyle display;
    std::array<widgets::ButtonColors, 4> roles{{
        {color::Role::surface_container_high, color::Role::on_surface},
        {color::Role::secondary_container, color::Role::on_secondary_container},
        {color::Role::tertiary_container, color::Role::on_tertiary_container},
        {color::Role::primary, color::Role::on_primary},
    }};
    std::array<KeySpec, key_count> keys{{
        {"AC", std::nullopt, Command::all_clear, KeyRole::clear, {{0, 0}, {0, 3}, false}},
        {"( )", std::nullopt, Command::parentheses, KeyRole::operation, {{0, 1}, {1, 3}, false}},
        {"%", std::nullopt, Command::percent, KeyRole::operation, {{0, 2}, {2, 3}, false}},
        {"÷", std::nullopt, Command::divide, KeyRole::operation, {{0, 3}, {3, 3}, false}},
        {"7", std::nullopt, Command::digit_7, KeyRole::digit, {{1, 0}, {0, 0}, false}},
        {"8", std::nullopt, Command::digit_8, KeyRole::digit, {{1, 1}, {0, 1}, false}},
        {"9", std::nullopt, Command::digit_9, KeyRole::digit, {{1, 2}, {0, 2}, false}},
        {"×", std::nullopt, Command::multiply, KeyRole::operation, {{1, 3}, {0, 4}, false}},
        {"4", std::nullopt, Command::digit_4, KeyRole::digit, {{2, 0}, {1, 0}, false}},
        {"5", std::nullopt, Command::digit_5, KeyRole::digit, {{2, 1}, {1, 1}, false}},
        {"6", std::nullopt, Command::digit_6, KeyRole::digit, {{2, 2}, {1, 2}, false}},
        {"−", std::nullopt, Command::subtract, KeyRole::operation, {{2, 3}, {1, 4}, false}},
        {"1", std::nullopt, Command::digit_1, KeyRole::digit, {{3, 0}, {2, 0}, false}},
        {"2", std::nullopt, Command::digit_2, KeyRole::digit, {{3, 1}, {2, 1}, false}},
        {"3", std::nullopt, Command::digit_3, KeyRole::digit, {{3, 2}, {2, 2}, false}},
        {"+", std::nullopt, Command::add, KeyRole::operation, {{3, 3}, {2, 4}, false}},
        {"0", std::nullopt, Command::digit_0, KeyRole::digit, {{4, 0}, {3, 0}, false}},
        {".", std::nullopt, Command::point, KeyRole::digit, {{4, 1}, {3, 1}, false}},
        {"", icons::Symbol::backspace, Command::backspace, KeyRole::digit, {{4, 2}, {3, 2}, false}},
        {"=", std::nullopt, Command::equals, KeyRole::equals, {{4, 3}, {3, 4}, false}},
        {"√", std::nullopt, Command::square_root, KeyRole::operation, {{0, 0}, {0, 0}, true}, LabelSwitch::inverse, "x²"},
        {"Rad", std::nullopt, Command::angle_unit, KeyRole::operation, {{0, 0}, {0, 1}, true}, LabelSwitch::angle_unit, "Deg"},
        {"Inv", std::nullopt, Command::inverse, KeyRole::operation, {{0, 0}, {0, 2}, true}},
        {"π", std::nullopt, Command::pi, KeyRole::operation, {{0, 0}, {1, 0}, true}},
        {"sin", std::nullopt, Command::sine, KeyRole::operation, {{0, 0}, {1, 1}, true}, LabelSwitch::inverse, "asin"},
        {"e", std::nullopt, Command::euler, KeyRole::operation, {{0, 0}, {1, 2}, true}},
        {"^", std::nullopt, Command::power, KeyRole::operation, {{0, 0}, {2, 0}, true}},
        {"cos", std::nullopt, Command::cosine, KeyRole::operation, {{0, 0}, {2, 1}, true}, LabelSwitch::inverse, "acos"},
        {"ln", std::nullopt, Command::natural_log, KeyRole::operation, {{0, 0}, {2, 2}, true}, LabelSwitch::inverse, "e^x"},
        {"!", std::nullopt, Command::factorial, KeyRole::operation, {{0, 0}, {3, 0}, true}},
        {"tan", std::nullopt, Command::tangent, KeyRole::operation, {{0, 0}, {3, 1}, true}, LabelSwitch::inverse, "atan"},
        {"log", std::nullopt, Command::common_log, KeyRole::operation, {{0, 0}, {3, 2}, true}, LabelSwitch::inverse, "10^x"},
    }};
    std::array<KeyBinding, 25> bindings{{
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
        {Key::caret, Command::power},       {Key::exclamation, Command::factorial},
        {Key::backspace, Command::backspace}, {Key::escape, Command::clear},
        {Key::delete_forward, Command::all_clear},
    }};
};

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

struct KeyView {
    widgets::Button button;
    const KeySpec* spec;
    AnimatedBox box;
    motion::Animated opacity;
    bool shown = true;
    double release_in = -1.0;
    std::string label;
};

}

int main(int argc, char** argv) {
    try {
        const Config config;
        const text::FontStack fonts = sdl3::font_stack_from_arguments(argc, argv);
        const icons::IconFont icon_font = sdl3::bundled_icon_font();
        const color::ColorScheme scheme = color::spec_2025().build(config.scheme);
        const auto& size_tokens = tokens::button_size(config.key_size);
        const double label_size = tokens::type_style(size_tokens.label).size_sp;

        examples::calculator::Engine engine;
        examples::calculator::Display display(config.display, fonts);
        examples::calculator::KeypadLayout layout(config.layout);
        AnimatedBox display_box(config.move_spring);

        std::vector<KeyView> keys;
        std::vector<KeyPlacement> placements;
        keys.reserve(config.keys.size());
        for (const KeySpec& spec : config.keys) {
            std::optional<icons::Icon> icon;
            if (spec.icon) {
                icon = icons::Icon{icon_font, *spec.icon};
            }
            keys.push_back({widgets::Button({spec.label, ButtonStyle::tonal, config.key_size, ButtonShape::round, icon,
                                             config.roles[static_cast<std::size_t>(spec.role)]},
                                            fonts),
                            &spec, AnimatedBox(config.move_spring), motion::Animated(1.0, config.fade_spring), true, -1.0, spec.label});
            placements.push_back(spec.placement);
        }

        auto refresh = [&] {
            display.show(engine.formula(), engine.result(), engine.focus() == examples::calculator::Focus::result,
                         engine.error() != examples::calculator::Error::none);
            for (KeyView& k : keys) {
                const bool alternate = (k.spec->label_switch == LabelSwitch::inverse && engine.inverse()) ||
                                       (k.spec->label_switch == LabelSwitch::angle_unit && engine.angle_unit() == AngleUnit::radians);
                const std::string label = alternate ? k.spec->alternate : k.spec->label;
                if (label != k.label) {
                    k.label = label;
                    k.button.set_label(label);
                }
            }
        };
        refresh();

        bool first = true;
        examples::calculator::Arrangement arrangement{};
        bool scientific = false;
        auto update = [&](const sdl3::Frame& frame) {
            const examples::calculator::KeypadFrame placed = layout.place(frame.width_dp, frame.height_dp, placements);
            const bool switched = !first && (placed.arrangement != arrangement || placed.scientific != scientific);
            arrangement = placed.arrangement;
            scientific = placed.scientific;
            for (std::size_t i = 0; i < keys.size(); ++i) {
                KeyView& k = keys[i];
                if (k.shown && !placed.shown[i]) {
                    k.button.cancel_interaction();
                }
                k.shown = placed.shown[i];
                if (first) {
                    k.box.snap_to(placed.keys[i]);
                    k.opacity.snap_to(k.shown ? 1.0 : 0.0);
                } else {
                    k.box.move_to(placed.keys[i], switched);
                    k.opacity.animate_to(k.shown ? 1.0 : 0.0);
                }
            }
            if (first) {
                display_box.snap_to(placed.display);
            } else {
                display_box.move_to(placed.display, switched);
            }
            display.set_badge(std::string(engine.angle_label()), scientific);
            first = false;

            display_box.step(frame.dt_seconds);
            display.arrange(display_box.rect());
            for (KeyView& k : keys) {
                k.box.step(frame.dt_seconds);
                k.opacity.step(frame.dt_seconds);
                const render::Rect r = k.box.rect();
                k.button.set_size(r.width, r.height);
                k.button.set_origin({r.x, r.y});
                const double side = std::min(r.width, r.height);
                k.button.set_content_scale(config.label_ratio * side / label_size, config.icon_ratio * side / size_tokens.icon_size);
            }

            bool changed = false;
            for (const auto& e : frame.pointer) {
                for (KeyView& k : keys) {
                    if (k.shown && k.button.handle(e)) {
                        engine.apply(k.spec->command);
                        changed = true;
                    }
                }
            }
            for (const auto& e : frame.keys) {
                if (const auto command = command_for(config, e)) {
                    engine.apply(*command);
                    changed = true;
                    for (KeyView& k : keys) {
                        if (k.shown && k.spec->command == *command) {
                            const render::Rect r = k.box.rect();
                            k.button.handle({input::PointerAction::down, r.x + r.width / 2.0, r.y + r.height / 2.0});
                            k.release_in = config.key_press_seconds;
                        }
                    }
                }
            }
            for (KeyView& k : keys) {
                if (k.release_in >= 0.0) {
                    k.release_in -= frame.dt_seconds;
                    if (k.release_in < 0.0) {
                        k.button.cancel_interaction();
                    }
                }
            }
            if (changed) {
                refresh();
            }
            display.step(frame.dt_seconds);
            for (KeyView& k : keys) {
                k.button.step(frame.dt_seconds);
            }
            return scheme[config.background];
        };

        auto paint = [&](render::Canvas& canvas, const sdl3::Frame&) {
            display.paint(canvas, scheme);
            bool animating = display.animating() || !display_box.settled();
            for (const KeyView& k : keys) {
                const double opacity = k.opacity.value();
                if (opacity > 0.0) {
                    k.button.paint(canvas, opacity < 1.0 ? color::faded(scheme, opacity) : scheme);
                }
                animating = animating || k.button.animating() || !k.box.settled() || !k.opacity.settled() || k.release_in >= 0.0;
            }
            return animating;
        };

        sdl3::run_app(config.window, examples::window_canvas(), update, paint);
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
