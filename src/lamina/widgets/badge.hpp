#pragma once

#include <optional>
#include <string>

#include "lamina/color/scheme.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/layout.hpp"
#include "lamina/tokens/badge.hpp"

namespace lamina::widgets {

struct BadgeSpec {
    std::string text = {};
    std::string accessible_description = {};
};

std::string badge_count(long long count, long long max = tokens::badge_tokens.max_count);

render::Rect badge_rect(const render::Rect& anchor, layout::Size badge, bool has_content,
                        const std::optional<render::Rect>& bounds = std::nullopt);

class Badge {
public:
    Badge(BadgeSpec spec, const text::FontStack& fonts);

    const BadgeSpec& spec() const { return spec_; }
    void set_spec(BadgeSpec spec);
    bool small() const { return spec_.text.empty(); }
    layout::Size size() const;
    std::string description() const { return spec_.accessible_description.empty() ? spec_.text : spec_.accessible_description; }

    render::Rect place(const render::Rect& anchor, const std::optional<render::Rect>& bounds = std::nullopt) const;
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme, const render::Rect& anchor,
               const std::optional<render::Rect>& bounds = std::nullopt, double opacity = 1.0) const;

private:
    BadgeSpec spec_;
    text::FontStack fonts_;
    text::TextLine line_;
};

void paint_badge(render::Canvas& canvas, const color::ColorScheme& scheme, const std::optional<Badge>& badge, const render::Rect& anchor,
                 const std::optional<render::Rect>& bounds = std::nullopt);

}
