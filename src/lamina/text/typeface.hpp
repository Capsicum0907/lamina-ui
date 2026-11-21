#pragma once

#include <array>
#include <optional>
#include <span>
#include <string_view>

#include "lamina/resources/resource.hpp"
#include "lamina/text/font.hpp"

namespace lamina::text {

enum class BundledFont {
    google_sans_flex,
    roboto_flex,
    noto_sans_jp,
};

struct BundledFontSpec {
    BundledFont font;
    std::string_view id;
    std::string_view family;
};

inline constexpr std::array<BundledFontSpec, 3> bundled_fonts{{
    {BundledFont::google_sans_flex, "google_sans_flex", "Google Sans Flex"},
    {BundledFont::roboto_flex, "roboto_flex", "Roboto Flex"},
    {BundledFont::noto_sans_jp, "noto_sans_jp", "Noto Sans JP"},
}};

struct EmbeddedFont {
    BundledFont font;
    const resources::Resource* resource;
};

enum class Typeface {
    google_sans_flex,
    roboto_flex,
    system,
};

struct TypefaceSpec {
    Typeface typeface;
    std::string_view name;
    std::string_view key;
    std::span<const BundledFont> chain;
    bool system_fallback;
};

inline constexpr BundledFont google_sans_flex_chain[] = {BundledFont::google_sans_flex, BundledFont::noto_sans_jp};
inline constexpr BundledFont roboto_flex_chain[] = {BundledFont::roboto_flex, BundledFont::noto_sans_jp};

inline constexpr std::array<TypefaceSpec, 3> typefaces{{
    {Typeface::google_sans_flex, "Google Sans Flex", "google-sans-flex", google_sans_flex_chain, true},
    {Typeface::roboto_flex, "Roboto Flex", "roboto-flex", roboto_flex_chain, true},
    {Typeface::system, "OS fonts", "os", {}, true},
}};

inline constexpr Typeface default_typeface = Typeface::google_sans_flex;

const BundledFontSpec& bundled_font_spec(BundledFont font);
const TypefaceSpec& typeface_spec(Typeface typeface);
std::optional<Typeface> typeface_from_key(std::string_view key);
const resources::Resource* find_embedded(std::span<const EmbeddedFont> embedded, BundledFont font);
bool typeface_available(Typeface typeface, std::span<const EmbeddedFont> embedded);
FontStack typeface_stack(Typeface typeface, std::span<const EmbeddedFont> embedded, const FontStack& system_fonts);

}
