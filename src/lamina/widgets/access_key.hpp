#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

#include "lamina/color/argb.hpp"
#include "lamina/input/events.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/text/layout.hpp"

namespace lamina::widgets {

std::optional<input::Key> access_key_for(std::string_view label, std::optional<input::Key> explicit_key);

struct AccessKeyMatch {
    std::size_t index;
    bool unique;
};

std::optional<AccessKeyMatch> match_access_key(const input::KeyEvent& e, std::span<const std::optional<input::Key>> keys,
                                               std::optional<std::size_t> active);

struct AccessKeyLabel {
    std::optional<text::TextLine> suffix;
    double underline_start;
    double underline_end;
    text::Underline underline;

    double suffix_width() const { return suffix ? suffix->width : 0.0; }
};

AccessKeyLabel access_key_label(const text::FontStack& fonts, std::string_view label, const text::TextLine& line, input::Key key);

void paint_access_key(render::Canvas& canvas, const text::FontStack& fonts, const AccessKeyLabel& mark, const text::TextLine& line,
                      render::Point top_left, color::Argb color);

}
