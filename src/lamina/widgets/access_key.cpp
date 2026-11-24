#include "lamina/style/style.hpp"
#include "lamina/widgets/access_key.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "lamina/tokens/access_key.hpp"

namespace lamina::widgets {

namespace {

const tokens::AccessKeyTokens& t() {
    return current_style().access_key;
}

constexpr char32_t ascii_lower(char32_t c) {
    return c >= U'A' && c <= U'Z' ? c - U'A' + U'a' : c;
}

constexpr char32_t ascii_upper(char32_t c) {
    return c >= U'a' && c <= U'z' ? c - U'a' + U'A' : c;
}

constexpr bool ascii_alphanumeric(char32_t c) {
    return (c >= U'a' && c <= U'z') || (c >= U'0' && c <= U'9');
}

}

std::optional<input::Key> access_key_for(std::string_view label, std::optional<input::Key> explicit_key) {
    if (explicit_key) {
        const std::optional<char32_t> c = input::character_for_key(*explicit_key);
        if (!c || !ascii_alphanumeric(*c)) {
            throw std::invalid_argument("an access key must be a letter or a digit");
        }
        return explicit_key;
    }
    const std::u32string text = text::decode_utf8(label);
    if (text.empty() || !ascii_alphanumeric(ascii_lower(text.front()))) {
        return std::nullopt;
    }
    return input::key_for_character(ascii_lower(text.front()));
}

std::optional<AccessKeyMatch> match_access_key(const input::KeyEvent& e, std::span<const std::optional<input::Key>> keys,
                                               std::optional<std::size_t> active) {
    if (e.shift || e.ctrl || e.alt || keys.empty()) {
        return std::nullopt;
    }
    const auto matches = [&](std::size_t i) { return keys[i] == e.key; };
    const std::size_t count = static_cast<std::size_t>(std::count(keys.begin(), keys.end(), std::optional<input::Key>(e.key)));
    if (count == 0) {
        return std::nullopt;
    }
    const std::size_t start = active ? *active + 1 : 0;
    for (std::size_t n = 0; n < keys.size(); ++n) {
        const std::size_t i = (start + n) % keys.size();
        if (matches(i)) {
            return AccessKeyMatch{i, count == 1};
        }
    }
    return std::nullopt;
}

AccessKeyLabel access_key_label(const text::FontStack& fonts, std::string_view label, const text::TextLine& line, input::Key key) {
    const char32_t c = *input::character_for_key(key);
    AccessKeyLabel mark{std::nullopt, 0.0, 0.0, text::underline(fonts, line.style)};
    mark.underline.thickness = std::max(mark.underline.thickness, t().min_underline_thickness);
    const std::u32string text = text::decode_utf8(label);
    const auto found = std::find_if(text.begin(), text.end(), [c](char32_t x) { return ascii_lower(x) == c; });
    if (found != text.end()) {
        const std::size_t i = static_cast<std::size_t>(found - text.begin());
        mark.underline_start = text::caret_x(line, i);
        mark.underline_end = text::caret_x(line, i + 1);
        return mark;
    }
    const std::u32string suffix{t().suffix_open, ascii_upper(c), t().suffix_close};
    mark.suffix = text::layout_line(fonts, text::encode_utf8(suffix), line.style);
    mark.underline_start = line.width + text::caret_x(*mark.suffix, 1);
    mark.underline_end = line.width + text::caret_x(*mark.suffix, 2);
    return mark;
}

void paint_access_key(render::Canvas& canvas, const text::FontStack& fonts, const AccessKeyLabel& mark, const text::TextLine& line,
                      render::Point top_left, color::Argb color) {
    const double baseline = top_left.y + line.baseline;
    if (mark.suffix) {
        text::draw_line(canvas, fonts, *mark.suffix, {top_left.x + line.width, baseline - mark.suffix->baseline}, color);
    }
    canvas.fill_rect({top_left.x + mark.underline_start, baseline + mark.underline.offset, mark.underline_end - mark.underline_start,
                      mark.underline.thickness},
                     color);
}

}
