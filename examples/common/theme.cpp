#include "common/theme.hpp"

#include <cmath>

#include "lamina/color/mix.hpp"
#include "lamina/tokens/motion.hpp"
#include "lamina/widgets/item_focus.hpp"

namespace lamina::examples {

ThemeFade::ThemeFade(const color::ColorScheme& initial)
    : from_(initial), to_(initial), progress_(1.0, tokens::motion_scheme.slow_effects()) {}

color::ColorScheme ThemeFade::current() const {
    return color::mix(from_, to_, progress_.value());
}

void ThemeFade::fade_to(const color::ColorScheme& target) {
    from_ = current();
    to_ = target;
    progress_.snap_to(0.0);
    progress_.animate_to(1.0);
}

void ThemeFade::step(double dt_seconds) {
    progress_.step(dt_seconds);
}

bool ThemeFade::settled() const {
    return progress_.settled();
}

SeedChips::SeedChips(ChipStyle style) : style_(style) {}

std::vector<layout::NodePtr> SeedChips::nodes() {
    std::vector<layout::NodePtr> out;
    for (std::size_t i = 0; i < theme_seeds.size(); ++i) {
        out.push_back(layout::fixed({style_.size, style_.size}, [this, i](const render::Rect& r) { rects_[i] = r; }));
    }
    return out;
}

std::optional<color::Argb> SeedChips::handle(const input::PointerEvent& e) {
    focus_.release_on_press(e);
    if (e.action != input::PointerAction::up) {
        return std::nullopt;
    }
    std::optional<color::Argb> chosen;
    for (std::size_t i = 0; i < theme_seeds.size(); ++i) {
        const double cx = rects_[i].x + rects_[i].width / 2.0;
        const double cy = rects_[i].y + rects_[i].height / 2.0;
        if (std::hypot(e.x_dp - cx, e.y_dp - cy) <= style_.size / 2.0 && i != selected_) {
            selected_ = i;
            chosen = theme_seeds[i];
        }
    }
    return chosen;
}

std::optional<color::Argb> SeedChips::handle(const input::KeyEvent& e) {
    if (!focus_.focused()) {
        return std::nullopt;
    }
    const std::optional<std::size_t> target = widgets::item_key_target(e, selected_, theme_seeds.size(), widgets::ItemAxis::horizontal);
    if (!target || *target == selected_) {
        return std::nullopt;
    }
    selected_ = *target;
    return theme_seeds[selected_];
}

void SeedChips::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    const double w = style_.ring_width;
    for (std::size_t i = 0; i < theme_seeds.size(); ++i) {
        const render::Rect c = rects_[i];
        if (i == selected_) {
            const render::Rect ring{c.x - 2 * w, c.y - 2 * w, c.width + 4 * w, c.height + 4 * w};
            canvas.fill_rounded_rect(ring, render::uniform_radii(ring.width / 2.0), scheme[style_.ring]);
            const render::Rect gap{c.x - w, c.y - w, c.width + 2 * w, c.height + 2 * w};
            canvas.fill_rounded_rect(gap, render::uniform_radii(gap.width / 2.0), scheme[style_.ring_gap]);
        }
        canvas.fill_rounded_rect(c, render::uniform_radii(style_.size / 2.0), theme_seeds[i]);
    }
    const render::Rect s = rects_[selected_];
    focus_.paint_circle(canvas, scheme, {s.x + s.width / 2.0, s.y + s.height / 2.0}, s.width / 2.0 + 2 * w);
}

}
