#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "lamina/render/path.hpp"
#include "lamina/resources/resource.hpp"

struct hb_blob_t;

namespace lamina::text {

struct ShapedGlyph {
    std::uint32_t glyph;
    std::uint32_t cluster;
    double advance_em;
    double offset_x_em;
    double offset_y_em;
};

struct Variation {
    std::uint32_t tag;
    float value;
};

constexpr std::uint32_t axis_tag(char a, char b, char c, char d) {
    return (static_cast<std::uint32_t>(static_cast<unsigned char>(a)) << 24) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(b)) << 16) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(c)) << 8) |
           static_cast<std::uint32_t>(static_cast<unsigned char>(d));
}

struct FontExtents {
    double ascender;
    double descender;
    double line_gap;
};

struct UnderlineMetrics {
    double position;
    double thickness;
};

struct Instance {
    int weight;
    double optical_size;
};

namespace axes {
inline constexpr std::uint32_t weight = axis_tag('w', 'g', 'h', 't');
inline constexpr std::uint32_t optical_size = axis_tag('o', 'p', 's', 'z');
inline constexpr std::uint32_t width = axis_tag('w', 'd', 't', 'h');
inline constexpr std::uint32_t slant = axis_tag('s', 'l', 'n', 't');
inline constexpr std::uint32_t roundness = axis_tag('R', 'O', 'N', 'D');
inline constexpr std::uint32_t grade = axis_tag('G', 'R', 'A', 'D');
}

class Font {
public:
    static std::shared_ptr<Font> from_file(const std::string& path, unsigned face_index = 0);
    static std::shared_ptr<Font> from_resource(const resources::Resource& resource, unsigned face_index = 0);
    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    std::shared_ptr<Font> with_variations(std::vector<Variation> pinned) const;
    const std::vector<Variation>& pinned_variations() const;
    bool has_axis(std::uint32_t tag) const;
    std::vector<Variation> variations(Instance instance) const;

    unsigned units_per_em() const;
    bool has_codepoint(char32_t codepoint) const;
    FontExtents extents_em() const;
    UnderlineMetrics underline_em() const;
    const render::Path& glyph_outline_em(std::uint32_t glyph, Instance instance) const;
    std::uint32_t glyph_for(char32_t codepoint) const;
    const render::Path& glyph_outline_em(std::uint32_t glyph, std::span<const Variation> variations) const;
    std::vector<ShapedGlyph> shape(std::u32string_view text, std::size_t cluster_base, Instance instance) const;

private:
    struct Impl;
    explicit Font(std::unique_ptr<Impl> impl);
    static std::shared_ptr<Font> from_blob(hb_blob_t* blob, unsigned face_index, const std::string& what);
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
std::string encode_utf8(std::u32string_view text);

}
