#include <doctest/doctest.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "lamina/text/layout.hpp"
#include "lamina/text/typeface.hpp"
#include "lamina/tokens/typography.hpp"
#include "test_res.hpp"

using namespace lamina;
using namespace lamina::text;

namespace {

struct LoadedFont {
    std::vector<unsigned char> bytes;
    resources::Resource resource;
};

LoadedFont* load(const char* name, const char* path) {
    auto* f = new LoadedFont;
    std::ifstream in(std::string(LAMINA_THIRD_PARTY_DIR) + "/" + path, std::ios::binary);
    f->bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    f->resource = {name, f->bytes};
    return f;
}

const LoadedFont& google_sans_flex() {
    static const LoadedFont* f = load("google_sans_flex", "google-sans-flex/GoogleSansFlex.ttf");
    return *f;
}

const LoadedFont& noto_sans_jp() {
    static const LoadedFont* f = load("noto_sans_jp", "noto-sans-jp/NotoSansJP.ttf");
    return *f;
}

std::vector<EmbeddedFont> all_embedded() {
    return {{BundledFont::google_sans_flex, &google_sans_flex().resource},
            {BundledFont::roboto_flex, &lamina::test::res::text_fonts::roboto_flex},
            {BundledFont::noto_sans_jp, &noto_sans_jp().resource}};
}

FontStack fake_system() {
    return FontStack({Font::from_resource(lamina::test::res::fonts::symbols)});
}

std::optional<float> axis_value(const std::vector<Variation>& variations, std::uint32_t tag) {
    for (const Variation& v : variations) {
        if (v.tag == tag) {
            return v.value;
        }
    }
    return std::nullopt;
}

constexpr char32_t settings_icon = 0xe8b8;

}

TEST_CASE("the default typeface is Google Sans Flex with Noto Sans JP, and every typeface has a unique key") {
    CHECK(default_typeface == Typeface::google_sans_flex);
    const TypefaceSpec& def = typeface_spec(default_typeface);
    REQUIRE(def.chain.size() == 2);
    CHECK(def.chain[0] == BundledFont::google_sans_flex);
    CHECK(def.chain[1] == BundledFont::noto_sans_jp);
    CHECK(typeface_spec(Typeface::roboto_flex).chain[0] == BundledFont::roboto_flex);
    CHECK(typeface_spec(Typeface::system).chain.empty());
    std::set<std::string_view> keys;
    for (const TypefaceSpec& s : typefaces) {
        CHECK(keys.insert(s.key).second);
        CHECK(typeface_from_key(s.key) == s.typeface);
    }
    CHECK_FALSE(typeface_from_key("google_sans").has_value());
    std::set<std::string_view> ids;
    for (const BundledFontSpec& s : bundled_fonts) {
        CHECK(ids.insert(s.id).second);
        CHECK(bundled_font_spec(s.font).family == s.family);
    }
}

TEST_CASE("a bundled typeface falls back from the Latin font to Noto Sans JP to the system fonts") {
    const std::vector<EmbeddedFont> embedded = all_embedded();
    const FontStack system = fake_system();
    for (Typeface t : {Typeface::google_sans_flex, Typeface::roboto_flex}) {
        CAPTURE(typeface_spec(t).name);
        const FontStack stack = typeface_stack(t, embedded, system);
        REQUIRE(stack.fonts().size() == 3);
        CHECK(stack.fonts()[2] == system.fonts()[0]);
        CHECK(stack.font_for(U'A') == 0);
        CHECK(stack.font_for(U'g') == 0);
        CHECK(stack.font_for(U'あ') == 1);
        CHECK(stack.font_for(U'日') == 1);
        CHECK(stack.font_for(U'ー') == 1);
        CHECK(stack.font_for(settings_icon) == 2);
    }
}

TEST_CASE("a bundled typeface without system fonts still has its embedded fonts") {
    const FontStack stack = typeface_stack(Typeface::google_sans_flex, all_embedded(), FontStack{});
    REQUIRE(stack.fonts().size() == 2);
    CHECK(stack.font_for(settings_icon) == 0);
}

TEST_CASE("the OS fonts typeface is exactly the system font stack") {
    const FontStack system = fake_system();
    const FontStack stack = typeface_stack(Typeface::system, {}, system);
    REQUIRE(stack.fonts().size() == 1);
    CHECK(stack.fonts()[0] == system.fonts()[0]);
    CHECK(typeface_available(Typeface::system, {}));
    CHECK_THROWS_AS(typeface_stack(Typeface::system, all_embedded(), FontStack{}), std::invalid_argument);
}

TEST_CASE("a typeface whose fonts are not embedded is reported, not replaced") {
    const std::vector<EmbeddedFont> only_latin{{BundledFont::google_sans_flex, &google_sans_flex().resource}};
    CHECK_FALSE(typeface_available(Typeface::google_sans_flex, only_latin));
    CHECK_FALSE(typeface_available(Typeface::roboto_flex, lamina::test::res::embedded_fonts));
    CHECK(typeface_available(Typeface::roboto_flex, all_embedded()));
    try {
        (void)typeface_stack(Typeface::google_sans_flex, only_latin, fake_system());
        FAIL("expected invalid_argument");
    } catch (const std::invalid_argument& ex) {
        CHECK(std::string(ex.what()).find("noto_sans_jp") != std::string::npos);
        CHECK(std::string(ex.what()).find("TEXT_FONTS") != std::string::npos);
    }
}

TEST_CASE("TEXT_FONTS embeds the font unchanged and lists it in embedded_fonts") {
    const resources::Resource& r = lamina::test::res::text_fonts::roboto_flex;
    CHECK(r.name == "roboto_flex");
    std::ifstream in(std::string(LAMINA_THIRD_PARTY_DIR) + "/roboto-flex/RobotoFlex.ttf", std::ios::binary);
    const std::vector<unsigned char> file{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    REQUIRE(r.data.size() == 1787292);
    CHECK(std::equal(file.begin(), file.end(), r.data.begin(), r.data.end()));
    REQUIRE(lamina::test::res::embedded_fonts.size() == 1);
    CHECK(lamina::test::res::embedded_fonts[0].font == BundledFont::roboto_flex);
    CHECK(lamina::test::res::embedded_fonts[0].resource == &r);
    CHECK(find_embedded(lamina::test::res::embedded_fonts, BundledFont::roboto_flex) == &r);
    CHECK(find_embedded(lamina::test::res::embedded_fonts, BundledFont::noto_sans_jp) == nullptr);
}

TEST_CASE("every type scale role sets wght to its weight and opsz to its size where the font has them") {
    const auto roboto = Font::from_resource(lamina::test::res::text_fonts::roboto_flex);
    const auto noto = Font::from_resource(noto_sans_jp().resource);
    CHECK(roboto->has_axis(axes::optical_size));
    CHECK_FALSE(noto->has_axis(axes::optical_size));
    for (std::size_t i = 0; i < tokens::type_role_count; ++i) {
        const auto role = static_cast<tokens::TypeRole>(i);
        const TextStyle style = tokens::text_style(tokens::type_style(role));
        CAPTURE(i);
        const std::vector<Variation> r = roboto->variations(instance_of(style));
        CHECK(axis_value(r, axes::weight) == doctest::Approx(style.weight));
        CHECK(axis_value(r, axes::optical_size) == doctest::Approx(style.size));
        const std::vector<Variation> n = noto->variations(instance_of(style));
        CHECK(axis_value(n, axes::weight) == doctest::Approx(style.weight));
        CHECK_FALSE(axis_value(n, axes::optical_size).has_value());
        if (tokens::is_emphasized(role)) {
            CHECK(style.weight > tokens::type_style(tokens::base_role(role)).weight);
        }
    }
}

TEST_CASE("pinned axes are kept and override the instance") {
    const auto gsf = Font::from_resource(google_sans_flex().resource);
    for (std::uint32_t tag : {axes::weight, axes::optical_size, axes::width, axes::slant, axes::roundness}) {
        CHECK(gsf->has_axis(tag));
    }
    const auto round = gsf->with_variations({{axes::roundness, 100.0f}, {axes::width, 90.0f}});
    const std::vector<Variation> v = round->variations({500, 14.0});
    CHECK(axis_value(v, axes::roundness) == doctest::Approx(100.0));
    CHECK(axis_value(v, axes::width) == doctest::Approx(90.0));
    CHECK(axis_value(v, axes::weight) == doctest::Approx(500.0));
    CHECK(axis_value(v, axes::optical_size) == doctest::Approx(14.0));
    const auto heavy = round->with_variations({{axes::weight, 800.0f}});
    CHECK(heavy->pinned_variations().size() == 3);
    CHECK(axis_value(heavy->variations({400, 14.0}), axes::weight) == doctest::Approx(800.0));
    CHECK(gsf->pinned_variations().empty());
    const std::uint32_t a = gsf->glyph_for(U'a');
    CHECK(gsf->shape(U"a", 0, {400, 14.0})[0].advance_em != doctest::Approx(round->shape(U"a", 0, {400, 14.0})[0].advance_em));
    CHECK(a != 0);
}

TEST_CASE("optical size changes the letters of a font with opsz and nothing else") {
    const auto gsf = Font::from_resource(google_sans_flex().resource);
    const double small = gsf->shape(U"a", 0, {400, 11.0})[0].advance_em;
    const double large = gsf->shape(U"a", 0, {400, 57.0})[0].advance_em;
    CHECK(small != doctest::Approx(large));
    CHECK(gsf->shape(U"a", 0, {400, 57.2})[0].advance_em == doctest::Approx(large));
    const auto noto = Font::from_resource(noto_sans_jp().resource);
    CHECK(noto->shape(U"あa", 0, {400, 11.0})[1].advance_em == doctest::Approx(noto->shape(U"あa", 0, {400, 57.0})[1].advance_em));
    CHECK(gsf->shape(U"a", 0, {700, 14.0})[0].advance_em > gsf->shape(U"a", 0, {400, 14.0})[0].advance_em);
}

TEST_CASE("an emphasized role lays out wider than its base role in Google Sans Flex") {
    const FontStack stack = typeface_stack(Typeface::google_sans_flex, all_embedded(), FontStack{});
    for (tokens::TypeRole role : {tokens::TypeRole::title_medium, tokens::TypeRole::label_large, tokens::TypeRole::headline_small}) {
        const TextStyle base = tokens::text_style(tokens::type_style(role));
        const TextStyle strong = tokens::text_style(tokens::type_style(tokens::emphasized(role)));
        CHECK(layout_line(stack, "Emphasized", strong).width > layout_line(stack, "Emphasized", base).width);
    }
}
