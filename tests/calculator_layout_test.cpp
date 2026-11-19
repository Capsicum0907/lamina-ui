#include <doctest/doctest.h>

#include <vector>

#include "calculator/keypad_layout.hpp"

using namespace lamina::examples::calculator;

namespace {

const std::vector<KeyPlacement> keys{
    {{0, 0}, {0, 0}, false},
    {{4, 3}, {3, 4}, false},
    {{0, 0}, {0, 0}, true},
    {{0, 0}, {3, 2}, true},
};

}

TEST_CASE("portrait keys are circles a quarter of the width wide, and the display takes the rest") {
    KeypadLayout layout;
    const KeypadFrame f = layout.place(400.0, 780.0, keys);
    CHECK(f.arrangement == Arrangement::portrait);
    CHECK_FALSE(f.scientific);
    CHECK(f.key_width == doctest::Approx(90.0));
    CHECK(f.key_height == doctest::Approx(90.0));
    CHECK(f.keys[0].x == doctest::Approx(8.0));
    CHECK(f.keys[1].x == doctest::Approx(8.0 + 3 * 98.0));
    CHECK(f.keys[1].y + f.keys[1].height == doctest::Approx(772.0));
    CHECK(f.display.y == doctest::Approx(8.0));
    CHECK(f.display.y + f.display.height == doctest::Approx(f.keypad.y - 8.0));
    CHECK(f.shown == std::vector<bool>{true, true, false, false});
    CHECK(f.keys[2].x + f.keys[2].width < f.keypad.x);
}

TEST_CASE("a portrait keypad limited by the height stays centered instead of stretching") {
    KeypadLayout layout;
    const KeypadFrame f = layout.place(560.0, 560.0, keys);
    CHECK(f.arrangement == Arrangement::portrait);
    CHECK(f.key_width == f.key_height);
    CHECK(f.keypad.x + f.keypad.width / 2.0 == doctest::Approx(280.0));
}

TEST_CASE("a wide window uses the landscape arrangement with pills no wider than the cap") {
    KeypadLayout layout;
    const KeypadFrame f = layout.place(1400.0, 520.0, keys);
    CHECK(f.arrangement == Arrangement::landscape);
    CHECK(f.key_width <= layout.settings().max_key_aspect * f.key_height + 1e-9);
    CHECK(f.key_width > f.key_height);
    CHECK(f.keypad.x + f.keypad.width / 2.0 == doctest::Approx(700.0));
    CHECK(f.keys[1].y == doctest::Approx(f.keypad.y + 3 * (f.key_height + 8.0)));
}

TEST_CASE("the scientific columns appear left of the basic keys when eight columns fit") {
    KeypadLayout layout;
    const KeypadFrame f = layout.place(1400.0, 520.0, keys);
    REQUIRE(f.scientific);
    CHECK(f.shown == std::vector<bool>{true, true, true, true});
    CHECK(f.keys[2].x == doctest::Approx(f.keypad.x));
    CHECK(f.keys[0].x == doctest::Approx(f.keypad.x + 3 * (f.key_width + 8.0)));
    KeypadLayout narrow;
    const KeypadFrame g = narrow.place(700.0, 520.0, keys);
    CHECK(g.arrangement == Arrangement::landscape);
    CHECK_FALSE(g.scientific);
    CHECK(g.shown == std::vector<bool>{true, true, false, false});
    CHECK(g.keys[0].x == doctest::Approx(g.keypad.x));
    CHECK(g.keys[2].x < g.keypad.x);
}

TEST_CASE("arrangement and scientific columns switch with hysteresis") {
    KeypadLayout layout;
    CHECK(layout.place(400.0, 780.0, keys).arrangement == Arrangement::portrait);
    CHECK(layout.place(550.0, 500.0, keys).arrangement == Arrangement::portrait);
    CHECK(layout.place(610.0, 500.0, keys).arrangement == Arrangement::landscape);
    CHECK(layout.place(550.0, 500.0, keys).arrangement == Arrangement::landscape);
    CHECK(layout.place(495.0, 500.0, keys).arrangement == Arrangement::portrait);

    KeypadLayout wide;
    const double h = 520.0;
    const double key = (h - 16.0 - 160.0 - 8.0 - 24.0) / 4.0;
    auto width_for = [&](double ratio) { return 16.0 + 8.0 * ratio * key + 7.0 * 8.0; };
    CHECK_FALSE(wide.place(width_for(1.3), h, keys).scientific);
    CHECK(wide.place(width_for(1.45), h, keys).scientific);
    CHECK(wide.place(width_for(1.3), h, keys).scientific);
    CHECK_FALSE(wide.place(width_for(1.1), h, keys).scientific);
}
