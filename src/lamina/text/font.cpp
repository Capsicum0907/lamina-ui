#include "lamina/text/font.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

#include <hb.h>
#include <hb-ot.h>

namespace lamina::text {

namespace {

constexpr hb_tag_t weight_axis = HB_TAG('w', 'g', 'h', 't');
constexpr char32_t replacement_character = 0xfffd;

struct OutlineSink {
    render::Path path;
    double scale;
};

render::Point em_point(const OutlineSink& s, float x, float y) {
    return {x * s.scale, -y * s.scale};
}

void on_move(hb_draw_funcs_t*, void* data, hb_draw_state_t*, float x, float y, void*) {
    auto& s = *static_cast<OutlineSink*>(data);
    s.path.move_to(em_point(s, x, y));
}

void on_line(hb_draw_funcs_t*, void* data, hb_draw_state_t*, float x, float y, void*) {
    auto& s = *static_cast<OutlineSink*>(data);
    s.path.line_to(em_point(s, x, y));
}

void on_quadratic(hb_draw_funcs_t*, void* data, hb_draw_state_t* st, float cx, float cy, float x, float y, void*) {
    auto& s = *static_cast<OutlineSink*>(data);
    const float x0 = st->current_x;
    const float y0 = st->current_y;
    const float c1x = x0 + 2.0f / 3.0f * (cx - x0);
    const float c1y = y0 + 2.0f / 3.0f * (cy - y0);
    const float c2x = x + 2.0f / 3.0f * (cx - x);
    const float c2y = y + 2.0f / 3.0f * (cy - y);
    s.path.cubic_to(em_point(s, c1x, c1y), em_point(s, c2x, c2y), em_point(s, x, y));
}

void on_cubic(hb_draw_funcs_t*, void* data, hb_draw_state_t*, float c1x, float c1y, float c2x, float c2y, float x,
              float y, void*) {
    auto& s = *static_cast<OutlineSink*>(data);
    s.path.cubic_to(em_point(s, c1x, c1y), em_point(s, c2x, c2y), em_point(s, x, y));
}

void on_close(hb_draw_funcs_t*, void* data, hb_draw_state_t*, void*) {
    static_cast<OutlineSink*>(data)->path.close();
}

hb_draw_funcs_t* draw_funcs() {
    static hb_draw_funcs_t* funcs = [] {
        hb_draw_funcs_t* f = hb_draw_funcs_create();
        hb_draw_funcs_set_move_to_func(f, on_move, nullptr, nullptr);
        hb_draw_funcs_set_line_to_func(f, on_line, nullptr, nullptr);
        hb_draw_funcs_set_quadratic_to_func(f, on_quadratic, nullptr, nullptr);
        hb_draw_funcs_set_cubic_to_func(f, on_cubic, nullptr, nullptr);
        hb_draw_funcs_set_close_path_func(f, on_close, nullptr, nullptr);
        hb_draw_funcs_make_immutable(f);
        return f;
    }();
    return funcs;
}

}

struct Font::Impl {
    hb_face_t* face = nullptr;
    hb_font_t* base = nullptr;
    bool variable_weight = false;
    mutable std::map<int, hb_font_t*> by_weight;
    mutable std::map<std::pair<int, std::uint32_t>, render::Path> outlines;
    using VariationKey = std::vector<std::pair<std::uint32_t, float>>;
    mutable std::map<VariationKey, hb_font_t*> by_variation;
    mutable std::map<std::pair<VariationKey, std::uint32_t>, render::Path> varied_outlines;

    ~Impl() {
        for (auto& [w, f] : by_weight) {
            hb_font_destroy(f);
        }
        for (auto& [k, f] : by_variation) {
            hb_font_destroy(f);
        }
        hb_font_destroy(base);
        hb_face_destroy(face);
    }

    hb_font_t* font_for_weight(int weight) const {
        if (!variable_weight) {
            return base;
        }
        auto it = by_weight.find(weight);
        if (it != by_weight.end()) {
            return it->second;
        }
        hb_font_t* f = hb_font_create(face);
        const hb_variation_t v{weight_axis, static_cast<float>(weight)};
        hb_font_set_variations(f, &v, 1);
        by_weight.emplace(weight, f);
        return f;
    }

    hb_font_t* font_for_variations(const VariationKey& key) const {
        auto it = by_variation.find(key);
        if (it != by_variation.end()) {
            return it->second;
        }
        hb_font_t* f = hb_font_create(face);
        std::vector<hb_variation_t> values;
        values.reserve(key.size());
        for (const auto& [tag, value] : key) {
            values.push_back({tag, value});
        }
        hb_font_set_variations(f, values.data(), static_cast<unsigned>(values.size()));
        by_variation.emplace(key, f);
        return f;
    }
};

Font::Font(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

Font::~Font() = default;

std::shared_ptr<Font> Font::from_file(const std::string& path, unsigned face_index) {
    hb_blob_t* blob = hb_blob_create_from_file_or_fail(path.c_str());
    if (!blob) {
        throw std::runtime_error("cannot read font: " + path);
    }
    auto impl = std::make_unique<Impl>();
    impl->face = hb_face_create(blob, face_index);
    hb_blob_destroy(blob);
    if (hb_face_get_glyph_count(impl->face) == 0) {
        throw std::runtime_error("not a usable font: " + path);
    }
    impl->base = hb_font_create(impl->face);
    hb_ot_var_axis_info_t info{};
    impl->variable_weight = hb_ot_var_find_axis_info(impl->face, weight_axis, &info);
    return std::shared_ptr<Font>(new Font(std::move(impl)));
}

unsigned Font::units_per_em() const {
    return hb_face_get_upem(impl_->face);
}

bool Font::has_codepoint(char32_t codepoint) const {
    hb_codepoint_t glyph = 0;
    return hb_font_get_nominal_glyph(impl_->base, codepoint, &glyph);
}

FontExtents Font::extents_em() const {
    hb_font_extents_t e{};
    hb_font_get_h_extents(impl_->base, &e);
    const double upem = units_per_em();
    return {e.ascender / upem, e.descender / upem, e.line_gap / upem};
}

const render::Path& Font::glyph_outline_em(std::uint32_t glyph, int weight) const {
    const int key_weight = impl_->variable_weight ? weight : 0;
    const auto key = std::make_pair(key_weight, glyph);
    auto it = impl_->outlines.find(key);
    if (it != impl_->outlines.end()) {
        return it->second;
    }
    OutlineSink sink{{}, 1.0 / units_per_em()};
    hb_font_draw_glyph(impl_->font_for_weight(weight), glyph, draw_funcs(), &sink);
    return impl_->outlines.emplace(key, std::move(sink.path)).first->second;
}

std::uint32_t Font::glyph_for(char32_t codepoint) const {
    hb_codepoint_t glyph = 0;
    return hb_font_get_nominal_glyph(impl_->base, codepoint, &glyph) ? glyph : 0;
}

const render::Path& Font::glyph_outline_em(std::uint32_t glyph, std::span<const Variation> variations) const {
    Impl::VariationKey key;
    key.reserve(variations.size());
    for (const auto& v : variations) {
        key.emplace_back(v.tag, v.value);
    }
    std::sort(key.begin(), key.end());
    auto outline_key = std::make_pair(key, glyph);
    auto it = impl_->varied_outlines.find(outline_key);
    if (it != impl_->varied_outlines.end()) {
        return it->second;
    }
    OutlineSink sink{{}, 1.0 / units_per_em()};
    hb_font_draw_glyph(impl_->font_for_variations(key), glyph, draw_funcs(), &sink);
    return impl_->varied_outlines.emplace(std::move(outline_key), std::move(sink.path)).first->second;
}

std::vector<ShapedGlyph> Font::shape(std::u32string_view text, std::size_t cluster_base, int weight) const {
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf32(buffer, reinterpret_cast<const std::uint32_t*>(text.data()), static_cast<int>(text.size()), 0,
                        static_cast<int>(text.size()));
    hb_buffer_guess_segment_properties(buffer);
    hb_shape(impl_->font_for_weight(weight), buffer, nullptr, 0);

    unsigned count = 0;
    const hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer, &count);
    const hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buffer, &count);
    const double upem = units_per_em();
    std::vector<ShapedGlyph> out;
    out.reserve(count);
    for (unsigned i = 0; i < count; ++i) {
        out.push_back({info[i].codepoint, static_cast<std::uint32_t>(cluster_base + info[i].cluster),
                       pos[i].x_advance / upem, pos[i].x_offset / upem, pos[i].y_offset / upem});
    }
    hb_buffer_destroy(buffer);
    return out;
}

std::size_t FontStack::font_for(char32_t codepoint) const {
    for (std::size_t i = 0; i < fonts_.size(); ++i) {
        if (fonts_[i]->has_codepoint(codepoint)) {
            return i;
        }
    }
    return 0;
}

std::u32string decode_utf8(std::string_view s) {
    std::u32string out;
    out.reserve(s.size());
    std::size_t i = 0;
    while (i < s.size()) {
        const auto b0 = static_cast<unsigned char>(s[i]);
        int length = 0;
        char32_t cp = 0;
        if (b0 < 0x80) {
            length = 1;
            cp = b0;
        } else if ((b0 & 0xe0) == 0xc0) {
            length = 2;
            cp = b0 & 0x1f;
        } else if ((b0 & 0xf0) == 0xe0) {
            length = 3;
            cp = b0 & 0x0f;
        } else if ((b0 & 0xf8) == 0xf0) {
            length = 4;
            cp = b0 & 0x07;
        } else {
            out.push_back(replacement_character);
            ++i;
            continue;
        }
        if (i + length > s.size()) {
            out.push_back(replacement_character);
            break;
        }
        bool valid = true;
        for (int k = 1; k < length; ++k) {
            const auto b = static_cast<unsigned char>(s[i + k]);
            if ((b & 0xc0) != 0x80) {
                valid = false;
                break;
            }
            cp = (cp << 6) | (b & 0x3f);
        }
        out.push_back(valid ? cp : replacement_character);
        i += valid ? length : 1;
    }
    return out;
}

std::string encode_utf8(std::u32string_view text) {
    std::string out;
    out.reserve(text.size());
    for (char32_t c : text) {
        if (c < 0x80) {
            out.push_back(static_cast<char>(c));
        } else if (c < 0x800) {
            out.push_back(static_cast<char>(0xc0 | (c >> 6)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3f)));
        } else if (c < 0x10000) {
            out.push_back(static_cast<char>(0xe0 | (c >> 12)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3f)));
        } else {
            out.push_back(static_cast<char>(0xf0 | (c >> 18)));
            out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3f)));
        }
    }
    return out;
}

}
