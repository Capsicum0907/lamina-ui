#include <doctest/doctest.h>

#include <algorithm>
#include <cstdlib>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "lamina/builtin_resources.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/text/font.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;

namespace {

constexpr int instancing_gap = 8;

std::map<std::string, char32_t> codepoints() {
    std::map<std::string, char32_t> table;
    std::ifstream in(LAMINA_SYMBOLS_CODEPOINTS);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream fields(line);
        std::string name;
        std::string code;
        if (fields >> name >> code) {
            table.emplace(name, static_cast<char32_t>(std::stoul(code, nullptr, 16)));
        }
    }
    return table;
}

std::vector<std::byte> file_bytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::vector<char> raw{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    std::vector<std::byte> out(raw.size());
    std::transform(raw.begin(), raw.end(), out.begin(), [](char c) { return static_cast<std::byte>(c); });
    return out;
}

std::vector<color::Argb> pixels(const icons::IconFont& font, icons::Symbol symbol, const icons::IconStyle& style) {
    thorvg::Canvas canvas(48, 48);
    canvas.clear(0xffffffff);
    font.draw(canvas, symbol, {0.0, 0.0}, 48.0, 0xff000000, style);
    std::vector<color::Argb> out;
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            out.push_back(canvas.pixel(x, y));
        }
    }
    return out;
}

int max_channel_gap(const std::vector<color::Argb>& a, const std::vector<color::Argb>& b) {
    int gap = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (int shift = 0; shift < 32; shift += 8) {
            gap = std::max(gap, std::abs(static_cast<int>((a[i] >> shift) & 0xff) - static_cast<int>((b[i] >> shift) & 0xff)));
        }
    }
    return gap;
}

const icons::IconFont& filled_font() {
    static const icons::IconFont font = icons::IconFont::from_resource(res::fonts::filled);
    return font;
}

}

TEST_CASE("the generated constants carry the codepoints of the codepoints file") {
    const auto table = codepoints();
    REQUIRE_FALSE(table.empty());
    CHECK(res::icons::settings.code == table.at("settings"));
    CHECK(res::icons::play_arrow.code == table.at("play_arrow"));
    CHECK(res::icons::delete_.code == table.at("delete"));
    CHECK(res::icons::class_.code == table.at("class"));
    CHECK(res::icons::_10k.code == table.at("10k"));
    CHECK(builtin::icons::close.code == table.at("close"));
}

TEST_CASE("all_icons keeps the listed order and adds lamina's own icons after them") {
    CHECK(res::all_icons.front() == res::icons::add);
    const auto ten_k = std::find(res::all_icons.begin(), res::all_icons.end(), res::icons::_10k);
    REQUIRE(ten_k != res::all_icons.end());
    REQUIRE(ten_k + 1 != res::all_icons.end());
    CHECK(*(ten_k + 1) == builtin::icons::close);
    for (auto it = ten_k + 2; it != res::all_icons.end(); ++it) {
        CHECK(std::count(builtin::all_icons.begin(), builtin::all_icons.end(), *it) == 1);
    }
    CHECK(res::icons::close == builtin::icons::close);
    for (icons::Symbol s : builtin::all_icons) {
        CHECK(std::count(res::all_icons.begin(), res::all_icons.end(), s) == 1);
    }
}

TEST_CASE("every icon is in every cut") {
    for (icons::Symbol s : res::all_icons) {
        CHECK(icon_font().has(s));
        CHECK(filled_font().has(s));
    }
}

TEST_CASE("a cut with the fill axis pinned at 1 draws as the variable font at fill 1") {
    icons::IconStyle filled;
    filled.fill = 1.0;
    const auto pinned = pixels(filled_font(), res::icons::favorite, {});
    const auto varied = pixels(icon_font(), res::icons::favorite, filled);
    const auto outlined = pixels(icon_font(), res::icons::favorite, {});
    CHECK(max_channel_gap(pinned, varied) <= instancing_gap);
    CHECK(max_channel_gap(pinned, outlined) > instancing_gap);
}

TEST_CASE("an embedded file holds the bytes of the file") {
    const std::vector<std::byte> disk = file_bytes(std::string(LAMINA_TEST_FONTS_DIR) + "/lamina-test-sans.ttf");
    const auto embedded = res::files::sans.bytes();
    REQUIRE_FALSE(disk.empty());
    CHECK(res::files::sans.name == "sans");
    CHECK(std::equal(disk.begin(), disk.end(), embedded.begin(), embedded.end()));
    CHECK(res::files::empty.data.empty());
}

TEST_CASE("a text font loads from an embedded file") {
    const auto from_resource = text::Font::from_resource(res::files::sans);
    const auto from_file = text::Font::from_file(std::string(LAMINA_TEST_FONTS_DIR) + "/lamina-test-sans.ttf");
    CHECK(from_resource->units_per_em() == from_file->units_per_em());
    CHECK(from_resource->glyph_for(U'A') == from_file->glyph_for(U'A'));
    CHECK(from_resource->glyph_for(U'A') != 0);
}

TEST_CASE("bytes that are not a font are refused") {
    CHECK_THROWS_AS(text::Font::from_resource(res::files::empty), std::runtime_error);
}

TEST_CASE("an icon font without lamina's own icons is refused") {
    CHECK_THROWS_AS(icons::IconFont::from_resource(res::files::sans), std::invalid_argument);
}
