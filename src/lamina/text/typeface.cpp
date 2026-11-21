#include "lamina/text/typeface.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace lamina::text {

const BundledFontSpec& bundled_font_spec(BundledFont font) {
    const auto it = std::find_if(bundled_fonts.begin(), bundled_fonts.end(), [font](const BundledFontSpec& s) { return s.font == font; });
    if (it == bundled_fonts.end()) {
        throw std::invalid_argument("unknown bundled font");
    }
    return *it;
}

const TypefaceSpec& typeface_spec(Typeface typeface) {
    const auto it = std::find_if(typefaces.begin(), typefaces.end(), [typeface](const TypefaceSpec& s) { return s.typeface == typeface; });
    if (it == typefaces.end()) {
        throw std::invalid_argument("unknown typeface");
    }
    return *it;
}

std::optional<Typeface> typeface_from_key(std::string_view key) {
    for (const TypefaceSpec& s : typefaces) {
        if (s.key == key) {
            return s.typeface;
        }
    }
    return std::nullopt;
}

const resources::Resource* find_embedded(std::span<const EmbeddedFont> embedded, BundledFont font) {
    for (const EmbeddedFont& e : embedded) {
        if (e.font == font && e.resource) {
            return e.resource;
        }
    }
    return nullptr;
}

bool typeface_available(Typeface typeface, std::span<const EmbeddedFont> embedded) {
    const TypefaceSpec& spec = typeface_spec(typeface);
    return std::all_of(spec.chain.begin(), spec.chain.end(), [embedded](BundledFont f) { return find_embedded(embedded, f) != nullptr; });
}

FontStack typeface_stack(Typeface typeface, std::span<const EmbeddedFont> embedded, const FontStack& system_fonts) {
    const TypefaceSpec& spec = typeface_spec(typeface);
    FontStack stack;
    for (BundledFont f : spec.chain) {
        const resources::Resource* resource = find_embedded(embedded, f);
        if (!resource) {
            throw std::invalid_argument("the typeface " + std::string(spec.name) + " needs the font " + std::string(bundled_font_spec(f).id) +
                                        ", which is not embedded: list it in TEXT_FONTS of lamina_add_resources");
        }
        stack.add(Font::from_resource(*resource));
    }
    if (spec.chain.empty() || spec.system_fallback) {
        for (const auto& font : system_fonts.fonts()) {
            stack.add(font);
        }
    }
    if (stack.empty()) {
        throw std::invalid_argument("the typeface " + std::string(spec.name) + " has no fonts: no system font was found");
    }
    return stack;
}

}
