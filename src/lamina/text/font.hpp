#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "lamina/render/path.hpp"

namespace lamina::text {

struct ShapedGlyph {
    std::uint32_t glyph;
    std::uint32_t cluster;
    double advance_em;
    double offset_x_em;
    double offset_y_em;
};

struct FontExtents {
    double ascender;
    double descender;
    double line_gap;
};

class Font {
public:
    static std::shared_ptr<Font> from_file(const std::string& path, unsigned face_index = 0);
    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    unsigned units_per_em() const;
    bool has_codepoint(char32_t codepoint) const;
    FontExtents extents_em() const;
    const render::Path& glyph_outline_em(std::uint32_t glyph, int weight) const;
    std::vector<ShapedGlyph> shape(std::u32string_view text, std::size_t cluster_base, int weight) const;

private:
    struct Impl;
    explicit Font(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};

class FontStack {
public:
    FontStack() = default;
    explicit FontStack(std::vector<std::shared_ptr<Font>> fonts) : fonts_(std::move(fonts)) {}

    void add(std::shared_ptr<Font> font) { fonts_.push_back(std::move(font)); }
    const std::vector<std::shared_ptr<Font>>& fonts() const { return fonts_; }
    bool empty() const { return fonts_.empty(); }

    std::size_t font_for(char32_t codepoint) const;

private:
    std::vector<std::shared_ptr<Font>> fonts_;
};

std::u32string decode_utf8(std::string_view utf8);

}
