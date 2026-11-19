#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <exception>
#include <numbers>
#include <optional>
#include <string>
#include <vector>

#include "common/animated_box.hpp"
#include "common/canvas.hpp"
#include "lamina/color/mix.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/motion/lerp.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/button.hpp"
#include "lamina/widgets/progress_indicator.hpp"
#include "lamina_sdl3/app.hpp"
#include "lamina_sdl3/resources.hpp"
#include "lamina_sdl3/system_fonts.hpp"
#include "timer/engine.hpp"
#include "timer/layout.hpp"
#include "timer/readout.hpp"

using namespace lamina;
using examples::AnimatedBox;
using examples::timer::State;
using input::Key;
using tokens::ButtonShape;
using tokens::ButtonStyle;

namespace {

enum class PadKey { digit, double_zero, backspace };

struct PadSpec {
    const char* label;
    std::optional<icons::Symbol> icon;
    PadKey kind;
    int digit;
};

struct PresetSpec {
    const char* label;
    int seconds;
};

struct ControlSpec {
    const char* label;
    std::optional<icons::Symbol> icon;
    widgets::ButtonColors colors;
    ButtonShape shape;
    double label_ratio;
    double icon_ratio;
    icons::IconStyle icon_style = {};
};

constexpr icons::IconStyle filled_icon{.fill = 1.0};

struct Config {
    sdl3::WindowOptions window{"lamina-ui timer", 400, 780};
    color::SchemeOptions scheme{.seed = color::default_scheme_options.seed, .dark = true, .variant = color::Variant::tonal_spot, .contrast = 0.0};
    color::Role background = color::Role::surface;
    examples::timer::LayoutSettings layout;
    tokens::ButtonSize button_size = tokens::ButtonSize::extra_large;
    widgets::ButtonColors key_colors{color::Role::surface_container_high, color::Role::on_surface};
    double key_label_ratio = 0.36;
    double key_icon_ratio = 0.32;
    std::array<PadSpec, examples::timer::key_count> pad{{
        {"1", std::nullopt, PadKey::digit, 1},
        {"2", std::nullopt, PadKey::digit, 2},
        {"3", std::nullopt, PadKey::digit, 3},
        {"4", std::nullopt, PadKey::digit, 4},
        {"5", std::nullopt, PadKey::digit, 5},
        {"6", std::nullopt, PadKey::digit, 6},
        {"7", std::nullopt, PadKey::digit, 7},
        {"8", std::nullopt, PadKey::digit, 8},
        {"9", std::nullopt, PadKey::digit, 9},
        {"00", std::nullopt, PadKey::double_zero, 0},
        {"0", std::nullopt, PadKey::digit, 0},
        {"", icons::Symbol::backspace, PadKey::backspace, 0},
    }};
    std::array<PresetSpec, examples::timer::preset_count> presets{{{"1m", 60}, {"5m", 300}, {"10m", 600}, {"15m", 900}}};
    widgets::ButtonColors preset_colors{color::Role::secondary_container, color::Role::on_secondary_container};
    double preset_label_ratio = 0.4;
    ControlSpec start{"", icons::Symbol::play_arrow, {color::Role::primary, color::Role::on_primary}, ButtonShape::square, 0.0, 0.42, filled_icon};
    ControlSpec toggle{"", icons::Symbol::pause, {color::Role::primary, color::Role::on_primary}, ButtonShape::square, 0.0, 0.5, filled_icon};
    ControlSpec add{"+1:00", std::nullopt, {color::Role::secondary_container, color::Role::on_secondary_container},
                    ButtonShape::round, 0.34, 0.0};
    ControlSpec reset{"", icons::Symbol::refresh, {color::Role::secondary_container, color::Role::on_secondary_container},
                      ButtonShape::round, 0.0, 0.42};
    icons::Symbol pause_icon = icons::Symbol::pause;
    icons::Symbol resume_icon = icons::Symbol::play_arrow;
    icons::Symbol stop_icon = icons::Symbol::stop;
    widgets::CircularProgressOptions ring{.thickness = 10.0, .track_thickness = 10.0, .gap = 8.0, .amplitude = 3.0, .wavelength = 42.0};
    widgets::ProgressColors running_colors{};
    widgets::ProgressColors finished_colors{color::Role::error, color::Role::error_container};
    color::Role time_color = color::Role::on_surface;
    color::Role finished_time_color = color::Role::error;
    examples::timer::EntryReadoutStyle readout;
    examples::timer::TimeTextStyle time;
    double pulse_seconds = 1.2;
    double pulse_scale = 0.025;
    double hidden_scale = 0.92;
    double key_press_seconds = 0.12;
    motion::SpringSpec move_spring = tokens::motion_scheme.default_spatial();
    motion::SpringSpec fade_spring = tokens::motion_scheme.default_effects();
    motion::SpringSpec screen_fade_spring = tokens::motion_scheme.slow_effects();
    motion::SpringSpec screen_scale_spring = tokens::motion_scheme.slow_spatial();
};

struct KeyBinding {
    Key key;
    std::size_t pad_index;
};

constexpr std::array<KeyBinding, 11> pad_bindings{{
    {Key::digit_1, 0}, {Key::digit_2, 1}, {Key::digit_3, 2}, {Key::digit_4, 3}, {Key::digit_5, 4}, {Key::digit_6, 5},
    {Key::digit_7, 6}, {Key::digit_8, 7}, {Key::digit_9, 8}, {Key::digit_0, 10}, {Key::backspace, 11},
}};

struct View {
    widgets::Button button;
    AnimatedBox box;
    double release_in = -1.0;

    void press_briefly(double seconds) {
        const render::Rect r = box.rect();
        button.handle({input::PointerAction::down, r.x + r.width / 2.0, r.y + r.height / 2.0});
        release_in = seconds;
    }
    void step(double dt) {
        if (release_in >= 0.0) {
            release_in -= dt;
            if (release_in < 0.0) {
                button.cancel_interaction();
            }
        }
        box.step(dt);
        button.step(dt);
    }
    bool animating() const { return button.animating() || !box.settled() || release_in >= 0.0; }
};

widgets::Button make_button(const char* label, std::optional<icons::Symbol> icon, widgets::ButtonColors colors, ButtonShape shape,
                            const icons::IconStyle& icon_style, tokens::ButtonSize size, const icons::IconFont& icon_font,
                            const text::FontStack& fonts) {
    std::optional<icons::Icon> i;
    if (icon) {
        i = icons::Icon{icon_font, *icon};
    }
    return widgets::Button({label, ButtonStyle::tonal, size, shape, i, colors, icon_style}, fonts);
}

void fit(View& v, double label_ratio, double icon_ratio, double label_size, double icon_size) {
    const render::Rect r = v.box.rect();
    v.button.set_size(r.width, r.height);
    v.button.set_origin({r.x, r.y});
    const double side = std::min(r.width, r.height);
    v.button.set_content_scale(label_ratio * side / label_size, icon_ratio * side / icon_size);
}

void place(View& v, const examples::timer::Box& b, bool first, bool switched) {
    if (first) {
        v.box.snap_to(b);
    } else {
        v.box.move_to(b, switched);
    }
}

void scale_about(render::Canvas& canvas, render::Point center, double k) {
    canvas.translate(center.x, center.y);
    canvas.scale(k);
    canvas.translate(-center.x, -center.y);
}

}

int main(int argc, char** argv) {
    try {
        const Config config;
        const text::FontStack fonts = sdl3::font_stack_from_arguments(argc, argv);
        const icons::IconFont icon_font = sdl3::bundled_icon_font();
        const color::ColorScheme scheme = color::spec_2025().build(config.scheme);
        const auto& size_tokens = tokens::button_size(config.button_size);
        const double label_size = tokens::type_style(size_tokens.label).size_sp;
        const double icon_size = size_tokens.icon_size;
        const auto clock_start = std::chrono::steady_clock::now();
        auto now_seconds = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - clock_start).count(); };

        examples::timer::Engine engine;
        examples::timer::TimerLayout layout(config.layout);
        examples::timer::EntryReadout readout(config.readout, fonts);
        examples::timer::TimeText time_text(config.time, fonts);
        AnimatedBox readout_box(config.move_spring);
        AnimatedBox ring_box(config.move_spring);
        AnimatedBox time_box(config.move_spring);
        widgets::CircularProgressIndicator ring(config.ring);

        auto view = [&](const char* label, std::optional<icons::Symbol> icon, widgets::ButtonColors colors, ButtonShape shape,
                        const icons::IconStyle& icon_style = {}) {
            return View{make_button(label, icon, colors, shape, icon_style, config.button_size, icon_font, fonts),
                        AnimatedBox(config.move_spring)};
        };
        auto control = [&](const ControlSpec& c) { return view(c.label, c.icon, c.colors, c.shape, c.icon_style); };
        std::vector<View> pad;
        for (const PadSpec& p : config.pad) {
            pad.push_back(view(p.label, p.icon, config.key_colors, ButtonShape::round));
        }
        std::vector<View> presets;
        for (const PresetSpec& p : config.presets) {
            presets.push_back(view(p.label, std::nullopt, config.preset_colors, ButtonShape::round));
        }
        View start = control(config.start);
        View toggle = control(config.toggle);
        View add = control(config.add);
        View reset = control(config.reset);

        motion::Animated screen_opacity(0.0, config.screen_fade_spring);
        motion::Animated screen_scale(0.0, config.screen_scale_spring);
        motion::Animated start_presence(0.0, config.move_spring);
        motion::Animated alarm(0.0, config.fade_spring);
        double pulse_phase = 0.0;
        bool running_screen = false;
        std::optional<State> shown_state;

        auto sync = [&](double now, bool snap) {
            const State state = engine.state(now);
            readout.show(engine.entry_digits(), engine.typed().size());
            const double presence = engine.entry_seconds() > 0 ? 1.0 : 0.0;
            const bool to_running = state != State::entry;
            if (to_running != running_screen) {
                for (View* v : {&start, &toggle, &add, &reset}) {
                    v->button.cancel_interaction();
                }
                for (auto* group : {&pad, &presets}) {
                    for (View& v : *group) {
                        v.button.cancel_interaction();
                    }
                }
                running_screen = to_running;
            }
            const double screen = running_screen ? 1.0 : 0.0;
            const double alarmed = state == State::finished ? 1.0 : 0.0;
            if (snap) {
                start_presence.snap_to(presence);
                screen_opacity.snap_to(screen);
                screen_scale.snap_to(screen);
                alarm.snap_to(alarmed);
            } else {
                start_presence.animate_to(presence);
                screen_opacity.animate_to(screen);
                screen_scale.animate_to(screen);
                alarm.animate_to(alarmed);
            }
            if (state != shown_state) {
                const bool square = state == State::running;
                toggle.button.set_shape(square ? ButtonShape::square : ButtonShape::round);
                const icons::Symbol icon = state == State::running ? config.pause_icon
                                           : state == State::finished ? config.stop_icon
                                                                      : config.resume_icon;
                toggle.button.set_icon(icons::Icon{icon_font, icon});
                ring.set_colors(state == State::finished ? config.finished_colors : config.running_colors);
                shown_state = state;
            }
            ring.set_wavy(state == State::running);
            if (running_screen) {
                time_text.show(engine.readout(now));
            }
        };

        bool first = true;
        examples::timer::Arrangement arrangement{};
        auto update = [&](const sdl3::Frame& frame) {
            const double now = now_seconds();
            const examples::timer::TimerFrame placed = layout.place(frame.width_dp, frame.height_dp);
            const bool switched = !first && placed.arrangement != arrangement;
            arrangement = placed.arrangement;
            for (std::size_t i = 0; i < pad.size(); ++i) {
                place(pad[i], placed.entry.keys[i], first, switched);
            }
            for (std::size_t i = 0; i < presets.size(); ++i) {
                place(presets[i], placed.entry.presets[i], first, switched);
            }
            place(start, placed.entry.start, first, switched);
            place(toggle, placed.running.toggle, first, switched);
            place(add, placed.running.add, first, switched);
            place(reset, placed.running.reset, first, switched);
            if (first) {
                readout_box.snap_to(placed.entry.readout);
                ring_box.snap_to(placed.running.ring);
                time_box.snap_to(placed.running.time);
            } else {
                readout_box.move_to(placed.entry.readout, switched);
                ring_box.move_to(placed.running.ring, switched);
                time_box.move_to(placed.running.time, switched);
            }

            const State before = engine.state(now);
            bool eased = false;
            auto press_pad = [&](std::size_t i) {
                const PadSpec& p = config.pad[i];
                switch (p.kind) {
                case PadKey::digit: engine.type_digit(p.digit); break;
                case PadKey::double_zero: engine.type_double_zero(); break;
                case PadKey::backspace: engine.backspace(); break;
                }
            };
            auto add_minute = [&] {
                if (engine.add_time(now)) {
                    ring.ease_progress(engine.progress(now));
                    eased = true;
                }
            };
            for (const auto& e : frame.pointer) {
                if (!running_screen) {
                    for (std::size_t i = 0; i < pad.size(); ++i) {
                        if (pad[i].button.handle(e)) {
                            press_pad(i);
                        }
                    }
                    for (std::size_t i = 0; i < presets.size(); ++i) {
                        if (presets[i].button.handle(e)) {
                            engine.set_entry_seconds(config.presets[i].seconds);
                        }
                    }
                    if (start_presence.target() > 0.0 && start.button.handle(e)) {
                        engine.start(now);
                    }
                } else {
                    if (toggle.button.handle(e)) {
                        engine.toggle(now);
                    }
                    if (add.button.handle(e)) {
                        add_minute();
                    }
                    if (reset.button.handle(e)) {
                        engine.reset();
                    }
                }
            }
            for (const auto& e : frame.keys) {
                if (e.ctrl) {
                    continue;
                }
                if (e.key == Key::enter || e.key == Key::space) {
                    const State s = engine.state(now);
                    if (s == State::entry && engine.entry_seconds() > 0) {
                        start.press_briefly(config.key_press_seconds);
                    } else if (s != State::entry) {
                        toggle.press_briefly(config.key_press_seconds);
                    }
                    engine.toggle(now);
                } else if (e.key == Key::escape) {
                    if (engine.state(now) == State::entry) {
                        engine.clear_entry();
                    } else {
                        reset.press_briefly(config.key_press_seconds);
                        engine.reset();
                    }
                } else if (engine.state(now) == State::entry) {
                    for (const KeyBinding& b : pad_bindings) {
                        if (b.key == e.key) {
                            press_pad(b.pad_index);
                            pad[b.pad_index].press_briefly(config.key_press_seconds);
                        }
                    }
                }
            }

            const State after = engine.state(now);
            if (before == State::entry && after != State::entry) {
                ring.set_progress(engine.progress(now));
            } else if (after != State::entry && !eased) {
                ring.set_progress(engine.progress(now));
            }
            sync(now, first);
            first = false;

            readout_box.step(frame.dt_seconds);
            ring_box.step(frame.dt_seconds);
            time_box.step(frame.dt_seconds);
            readout.arrange(readout_box.rect());
            const render::Rect rr = ring_box.rect();
            ring.set_size(rr.width, rr.height);
            ring.set_origin({rr.x, rr.y});
            time_text.arrange(time_box.rect());
            for (auto* group : {&pad, &presets}) {
                for (View& v : *group) {
                    v.step(frame.dt_seconds);
                }
            }
            for (View* v : {&start, &toggle, &add, &reset}) {
                v->step(frame.dt_seconds);
            }
            for (View& v : pad) {
                fit(v, config.key_label_ratio, config.key_icon_ratio, label_size, icon_size);
            }
            for (View& v : presets) {
                fit(v, config.preset_label_ratio, 0.0, label_size, icon_size);
            }
            fit(start, config.start.label_ratio, config.start.icon_ratio, label_size, icon_size);
            fit(toggle, config.toggle.label_ratio, config.toggle.icon_ratio, label_size, icon_size);
            fit(add, config.add.label_ratio, config.add.icon_ratio, label_size, icon_size);
            fit(reset, config.reset.label_ratio, config.reset.icon_ratio, label_size, icon_size);

            readout.step(frame.dt_seconds);
            time_text.step(frame.dt_seconds);
            ring.step(frame.dt_seconds);
            screen_opacity.step(frame.dt_seconds);
            screen_scale.step(frame.dt_seconds);
            start_presence.step(frame.dt_seconds);
            alarm.step(frame.dt_seconds);
            if (alarm.value() > 0.0) {
                pulse_phase = std::fmod(pulse_phase + frame.dt_seconds / config.pulse_seconds, 1.0);
            } else {
                pulse_phase = 0.0;
            }
            return scheme[config.background];
        };

        auto paint = [&](render::Canvas& canvas, const sdl3::Frame& frame) {
            const render::Point center{frame.width_dp / 2.0, frame.height_dp / 2.0};
            const double o = std::clamp(screen_opacity.value(), 0.0, 1.0);
            const double s = screen_scale.value();
            auto layer = [&](double opacity, double scale, auto&& draw) {
                if (opacity <= 0.0) {
                    return;
                }
                canvas.save();
                scale_about(canvas, center, scale);
                draw(opacity < 1.0 ? color::faded(scheme, opacity) : scheme);
                canvas.restore();
            };
            layer(1.0 - o, motion::lerp(1.0, config.hidden_scale, s), [&](const color::ColorScheme& faded) {
                readout.paint(canvas, faded);
                for (const View& v : presets) {
                    v.button.paint(canvas, faded);
                }
                for (const View& v : pad) {
                    v.button.paint(canvas, faded);
                }
                const double presence = std::max(0.0, start_presence.value());
                if (presence > 0.0) {
                    const render::Rect r = start.box.rect();
                    canvas.save();
                    scale_about(canvas, {r.x + r.width / 2.0, r.y + r.height / 2.0}, presence);
                    start.button.paint(canvas, presence < 1.0 ? color::faded(faded, std::min(1.0, presence)) : faded);
                    canvas.restore();
                }
            });
            layer(o, motion::lerp(config.hidden_scale, 1.0, s), [&](const color::ColorScheme& faded) {
                const render::Rect r = ring_box.rect();
                const double pulse = 1.0 + config.pulse_scale * alarm.value() * std::sin(2.0 * std::numbers::pi * pulse_phase);
                canvas.save();
                scale_about(canvas, {r.x + r.width / 2.0, r.y + r.height / 2.0}, pulse);
                ring.paint(canvas, faded);
                time_text.paint(canvas, color::mix(faded[config.time_color], faded[config.finished_time_color], alarm.value()));
                toggle.button.paint(canvas, faded);
                canvas.restore();
                add.button.paint(canvas, faded);
                reset.button.paint(canvas, faded);
            });

            const State state = engine.state(now_seconds());
            bool animating = state == State::running || alarm.value() > 0.0 || !alarm.settled() || readout.animating() ||
                             time_text.animating() || ring.animating() || !screen_opacity.settled() || !screen_scale.settled() ||
                             !start_presence.settled() || !readout_box.settled() || !ring_box.settled() || !time_box.settled();
            for (const auto* group : {&pad, &presets}) {
                for (const View& v : *group) {
                    animating = animating || v.animating();
                }
            }
            for (const View* v : {&start, &toggle, &add, &reset}) {
                animating = animating || v->animating();
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
