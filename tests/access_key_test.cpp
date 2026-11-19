#include <doctest/doctest.h>

#include <optional>
#include <stdexcept>
#include <vector>

#include "lamina/tokens/access_key.hpp"
#include "lamina/widgets/access_key.hpp"
#include "lamina/widgets/grouped_menu.hpp"
#include "lamina/widgets/menu.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::KeyEvent;

namespace {

constexpr layout::Size window{400.0, 600.0};
constexpr render::Rect anchor{20.0, 40.0, 40.0, 40.0};
constexpr const char* japanese = "\xe3\x81\x82\xe3\x81\x84\xe3\x81\x86";
constexpr const char* long_japanese =
    "\xe3\x81\x82\xe3\x81\x84\xe3\x81\x86\xe3\x81\x88\xe3\x81\x8a\xe3\x81\x82\xe3\x81\x84\xe3\x81\x86\xe3\x81\x88\xe3\x81\x8a";

KeyEvent press(Key k) {
    return {k, false};
}

text::TextStyle label_style() {
    return tokens::text_style(tokens::type_style(tokens::menu_tokens.label));
}

widgets::Menu make() {
    return widgets::Menu({{{"Edit"}, {"Share"}, {"Copy", std::nullopt, Key::p}, {"settings"}, {japanese, std::nullopt, Key::j}}}, fonts());
}

std::vector<color::Argb> paint(widgets::Menu& m) {
    thorvg::Canvas canvas(400, 600);
    canvas.clear(0xffffffff);
    settle(m);
    m.paint(canvas, scheme());
    std::vector<color::Argb> pixels;
    for (int y = 0; y < 600; ++y) {
        for (int x = 0; x < 400; ++x) {
            pixels.push_back(canvas.pixel(x, y));
        }
    }
    return pixels;
}

}

TEST_CASE("access key tokens") {
    const auto& t = tokens::access_key_tokens;
    CHECK(t.min_underline_thickness == 1.0);
    CHECK(t.suffix_open == U'(');
    CHECK(t.suffix_close == U')');
}

TEST_CASE("an item's access key is its explicit key, or else the first letter or digit of its label") {
    using widgets::access_key_for;
    CHECK(access_key_for("Share", Key::h) == Key::h);
    CHECK(access_key_for(japanese, Key::s) == Key::s);
    CHECK(access_key_for("Share", std::nullopt) == Key::s);
    CHECK(access_key_for("share", std::nullopt) == Key::s);
    CHECK(access_key_for("3D view", std::nullopt) == Key::digit_3);
    CHECK_FALSE(access_key_for(japanese, std::nullopt));
    CHECK_FALSE(access_key_for("(new)", std::nullopt));
    CHECK_FALSE(access_key_for("", std::nullopt));
    CHECK_THROWS_AS(access_key_for("Share", Key::enter), std::invalid_argument);
    CHECK_THROWS_AS(access_key_for("Share", Key::plus), std::invalid_argument);
}

TEST_CASE("a bare key matches its items, cycling when several share it") {
    using widgets::match_access_key;
    const std::vector<std::optional<Key>> keys{Key::e, Key::s, std::nullopt, Key::s, Key::c};
    const auto e = match_access_key(press(Key::e), keys, std::nullopt);
    REQUIRE(e);
    CHECK(e->index == 0);
    CHECK(e->unique);
    const auto s = match_access_key(press(Key::s), keys, std::nullopt);
    REQUIRE(s);
    CHECK(s->index == 1);
    CHECK_FALSE(s->unique);
    CHECK(match_access_key(press(Key::s), keys, 1)->index == 3);
    CHECK(match_access_key(press(Key::s), keys, 3)->index == 1);
    CHECK(match_access_key(press(Key::s), keys, 4)->index == 1);
    CHECK_FALSE(match_access_key(press(Key::x), keys, std::nullopt));
    CHECK_FALSE(match_access_key(KeyEvent{Key::e, true}, keys, std::nullopt));
    CHECK_FALSE(match_access_key(KeyEvent{Key::e, false, true}, keys, std::nullopt));
    CHECK_FALSE(match_access_key(KeyEvent{Key::e, false, false, false, true}, keys, std::nullopt));
    CHECK_FALSE(match_access_key(press(Key::e), {}, std::nullopt));
}

TEST_CASE("the access key underlines its first letter in the label, in either case") {
    const text::TextLine line = text::layout_line(fonts(), "Copy Paste", label_style());
    const widgets::AccessKeyLabel p = widgets::access_key_label(fonts(), "Copy Paste", line, Key::p);
    CHECK_FALSE(p.suffix);
    CHECK(p.suffix_width() == 0.0);
    CHECK(p.underline_start == doctest::Approx(text::caret_x(line, 2)));
    CHECK(p.underline_end == doctest::Approx(text::caret_x(line, 3)));
    const widgets::AccessKeyLabel c = widgets::access_key_label(fonts(), "Copy Paste", line, Key::c);
    CHECK(c.underline_start == 0.0);
    CHECK(c.underline_end == doctest::Approx(text::caret_x(line, 1)));
    CHECK(c.underline.thickness >= tokens::access_key_tokens.min_underline_thickness);
    CHECK(c.underline.offset > 0.0);
}

TEST_CASE("a label without the key's letter gets the key in brackets after it") {
    const text::TextLine line = text::layout_line(fonts(), japanese, label_style());
    const widgets::AccessKeyLabel s = widgets::access_key_label(fonts(), japanese, line, Key::s);
    REQUIRE(s.suffix);
    CHECK(s.suffix_width() == doctest::Approx(text::layout_line(fonts(), "(S)", label_style()).width));
    CHECK(s.underline_start > line.width);
    CHECK(s.underline_end < line.width + s.suffix_width());
}

TEST_CASE("the menu's items take explicit and first-letter access keys") {
    auto m = make();
    CHECK(m.access_key(0) == Key::e);
    CHECK(m.access_key(1) == Key::s);
    CHECK(m.access_key(2) == Key::p);
    CHECK(m.access_key(3) == Key::s);
    CHECK(m.access_key(4) == Key::j);
    widgets::Menu plain({{{japanese}}}, fonts());
    CHECK_FALSE(plain.access_key(0));
}

TEST_CASE("an access key that one item has chooses it and closes the menu, from either input source") {
    for (auto source : {input::InputSource::pointer, input::InputSource::keyboard}) {
        auto m = make();
        m.open(anchor, window, source);
        const widgets::MenuResponse r = m.handle(press(Key::p));
        CHECK(r.consumed);
        CHECK(r.chosen == 2u);
        CHECK_FALSE(m.is_open());
    }
    auto m = make();
    m.open(anchor, window);
    CHECK(m.handle(press(Key::j)).chosen == 4u);
}

TEST_CASE("an access key that several items share moves the focus between them without choosing") {
    auto m = make();
    m.open(anchor, window);
    widgets::MenuResponse r = m.handle(press(Key::s));
    CHECK(r.consumed);
    CHECK_FALSE(r.chosen);
    CHECK(m.focused_item() == 1u);
    CHECK(m.focus_shown());
    r = m.handle(press(Key::s));
    CHECK_FALSE(r.chosen);
    CHECK(m.focused_item() == 3u);
    m.handle(press(Key::s));
    CHECK(m.focused_item() == 1u);
    CHECK(m.is_open());
    CHECK(m.handle(press(Key::enter)).chosen == 1u);
}

TEST_CASE("a key no item has, a modified key or a held key does nothing") {
    auto m = make();
    m.open(anchor, window, input::InputSource::keyboard);
    const widgets::MenuResponse r = m.handle(press(Key::x));
    CHECK_FALSE(r.consumed);
    CHECK_FALSE(r.chosen);
    CHECK(m.focused_item() == 0u);
    CHECK_FALSE(m.handle(KeyEvent{Key::p, true}).chosen);
    CHECK_FALSE(m.handle(KeyEvent{Key::p, false, true}).chosen);
    CHECK_FALSE(m.handle(KeyEvent{Key::p, false, false, false, true}).chosen);
    CHECK_FALSE(m.handle(KeyEvent{Key::p, false, false, true}).chosen);
    CHECK(m.is_open());
    CHECK(m.focused_item() == 0u);
}

TEST_CASE("the menu shows its access keys at once when the keyboard opened it, and hides them when the pointer did") {
    auto m = make();
    m.open(anchor, window, input::InputSource::keyboard);
    CHECK(m.access_keys_shown());
    m.close();
    m.open(anchor, window);
    CHECK_FALSE(m.access_keys_shown());

    widgets::Menu keyed({{{japanese, std::nullopt, Key::s}}}, fonts());
    widgets::Menu plain({{{japanese}}}, fonts());
    keyed.open(anchor, window);
    plain.open(anchor, window);
    CHECK(paint(keyed) == paint(plain));
    keyed.close();
    plain.close();
    settle(keyed);
    settle(plain);
    keyed.open(anchor, window, input::InputSource::keyboard);
    plain.open(anchor, window, input::InputSource::keyboard);
    CHECK(paint(keyed) != paint(plain));
}

TEST_CASE("a pointer-opened menu fades its access keys in once a key moves within it") {
    widgets::Menu keyed({{{japanese, std::nullopt, Key::s}, {"Copy"}}}, fonts());
    widgets::Menu plain({{{japanese}, {"Copy", std::nullopt, Key::j}}}, fonts());
    keyed.open(anchor, window);
    plain.open(anchor, window);
    settle(keyed);
    settle(plain);
    CHECK_FALSE(keyed.access_keys_shown());
    CHECK(keyed.handle(press(Key::down)).consumed);
    CHECK(keyed.access_keys_shown());
    CHECK(keyed.animating());
    CHECK(paint(keyed) != paint(plain));
}

TEST_CASE("an access key pressed in a pointer-opened menu shows the access keys") {
    auto m = make();
    m.open(anchor, window);
    m.handle(press(Key::s));
    CHECK(m.access_keys_shown());
}

TEST_CASE("the grouped menu fades its access keys in once a key moves within it") {
    widgets::GroupedMenu m({{{{{long_japanese, std::nullopt, std::nullopt, std::nullopt, false, Key::s}}}}}, fonts());
    m.open(anchor, window);
    CHECK_FALSE(m.access_keys_shown());
    m.handle(press(Key::down));
    CHECK(m.access_keys_shown());
    CHECK(m.animating());
}

TEST_CASE("the menu is as wide with its access keys hidden as shown, with room for the brackets") {
    widgets::Menu keyed({{{long_japanese, std::nullopt, Key::s}}}, fonts());
    widgets::Menu plain({{{long_japanese}}}, fonts());
    CHECK(plain.width() > tokens::menu_tokens.item_min_width);
    CHECK(keyed.width() == doctest::Approx(plain.width() + text::layout_line(fonts(), "(S)", label_style()).width));
    keyed.open(anchor, window);
    const render::Rect by_pointer = keyed.bounds();
    keyed.close();
    keyed.open(anchor, window, input::InputSource::keyboard);
    CHECK(keyed.bounds().width == by_pointer.width);
    CHECK(keyed.bounds().x == by_pointer.x);
}

namespace {

widgets::GroupedMenu make_grouped() {
    return widgets::GroupedMenu({{{{{"List", std::nullopt, std::nullopt, std::nullopt, true}, {"Grid"}}, "Layout", false, widgets::MenuSelection::single},
                                  {{{"Bookmarked"}, {japanese, std::nullopt, std::nullopt, std::nullopt, false, Key::r}}, {}, false,
                                   widgets::MenuSelection::multiple},
                                  {{{"Share"}, {"Settings"}}}}},
                                fonts());
}

}

TEST_CASE("the grouped menu's items take access keys across its groups") {
    auto m = make_grouped();
    CHECK(m.access_key(0, 0) == Key::l);
    CHECK(m.access_key(0, 1) == Key::g);
    CHECK(m.access_key(1, 0) == Key::b);
    CHECK(m.access_key(1, 1) == Key::r);
    CHECK(m.access_key(2, 0) == Key::s);
    CHECK(m.access_key(2, 1) == Key::s);
}

TEST_CASE("an access key in the grouped menu does what a click on its item does") {
    auto m = make_grouped();
    m.open(anchor, window);
    widgets::GroupedMenuResponse r = m.handle(press(Key::g));
    CHECK(r.consumed);
    REQUIRE(r.chosen);
    CHECK(r.chosen->group == 0);
    CHECK(r.chosen->item == 1);
    CHECK(m.is_open());
    CHECK(m.selected(0, 1));
    CHECK_FALSE(m.selected(0, 0));
    CHECK(m.focused_item()->group == 0);
    CHECK(m.focused_item()->item == 1);
    m.handle(press(Key::r));
    CHECK(m.is_open());
    CHECK(m.selected(1, 1));
    m.handle(press(Key::r));
    CHECK_FALSE(m.selected(1, 1));
    m.handle(KeyEvent{Key::r, false, false, true});
    CHECK_FALSE(m.selected(1, 1));
    CHECK_FALSE(m.handle(press(Key::x)).chosen);
    r = m.handle(press(Key::s));
    CHECK_FALSE(r.chosen);
    CHECK(m.focused_item()->group == 2);
    CHECK(m.focused_item()->item == 0);
    m.handle(press(Key::s));
    CHECK(m.focused_item()->item == 1);
    CHECK(m.is_open());
    r = m.handle(press(Key::b));
    REQUIRE(r.chosen);
    CHECK(m.selected(1, 0));
    m.close();
    m.open(anchor, window, input::InputSource::keyboard);
    CHECK(m.access_keys_shown());
    r = m.handle(press(Key::l));
    CHECK(r.chosen->group == 0);
    CHECK(m.selected(0, 0));
    CHECK(m.is_open());
}

TEST_CASE("the grouped menu shows its access keys only when the keyboard opened it, at the same width") {
    auto keyed = [] {
        return widgets::GroupedMenu({{{{{long_japanese, std::nullopt, std::nullopt, std::nullopt, false, Key::s}}}}}, fonts());
    };
    widgets::GroupedMenu plain({{{{{long_japanese}}}}}, fonts());
    auto m = keyed();
    const auto style = tokens::text_style(tokens::type_style(tokens::grouped_menu_tokens.label));
    CHECK(m.width() == doctest::Approx(plain.width() + text::layout_line(fonts(), "(S)", style).width));
    m.open(anchor, window);
    CHECK_FALSE(m.access_keys_shown());
    const render::Rect by_pointer = m.bounds();
    m.close();
    m.open(anchor, window, input::InputSource::keyboard);
    CHECK(m.access_keys_shown());
    CHECK(m.bounds().width == by_pointer.width);
    CHECK(m.bounds().x == by_pointer.x);
}
