#include <doctest/doctest.h>

#include <memory>
#include <vector>

#include "lamina/widgets/button.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/text_field.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::PointerAction;

static_assert(widgets::TextInput<widgets::TextField>);
static_assert(widgets::Focusable<widgets::Button>);

namespace {

constexpr input::KeyEvent tab{input::Key::tab, false};
constexpr input::KeyEvent shift_tab{input::Key::tab, true};

struct Fields {
    std::vector<std::unique_ptr<widgets::TextField>> list;
    widgets::FocusManager focus;

    explicit Fields(int n) {
        for (int i = 0; i < n; ++i) {
            list.push_back(std::make_unique<widgets::TextField>(widgets::TextFieldOptions{.label = "Field"}, fonts()));
            list.back()->set_origin({0.0, 100.0 * i});
            focus.add(*list.back());
        }
    }

    int focused() const {
        int found = -1;
        for (std::size_t i = 0; i < list.size(); ++i) {
            if (list[i]->focused()) {
                CHECK(found == -1);
                found = static_cast<int>(i);
            }
        }
        return found;
    }
};

class Toggle {
public:
    bool focused() const { return focused_; }
    void set_focused(bool focused) { focused_ = focused; }

private:
    bool focused_ = false;
};

class Gated {
public:
    bool focused() const { return focused_; }
    void set_focused(bool focused) { focused_ = focused; }
    bool can_focus() const { return enabled; }
    bool enabled = true;

private:
    bool focused_ = false;
};

static_assert(widgets::FocusGated<Gated>);
static_assert(!widgets::FocusGated<Toggle>);

}

TEST_CASE("Tab moves the focus forward and Shift+Tab backward, wrapping at the ends") {
    Fields f(3);
    f.list[0]->set_focused(true);
    CHECK(f.focus.handle(tab));
    CHECK(f.focused() == 1);
    CHECK(f.focus.handle(tab));
    CHECK(f.focus.handle(tab));
    CHECK(f.focused() == 0);
    CHECK(f.focus.handle(shift_tab));
    CHECK(f.focused() == 2);
    CHECK(f.focus.move_focus(widgets::FocusDirection::previous));
    CHECK(f.focused() == 1);
}

TEST_CASE("Tab with nothing focused focuses the first widget and Shift+Tab the last; other keys are left to the widgets") {
    Fields f(3);
    CHECK(f.focus.handle(tab));
    CHECK(f.focused() == 0);
    f.focus.clear_focus();
    CHECK(f.focus.handle(shift_tab));
    CHECK(f.focused() == 2);
    CHECK_FALSE(f.focus.handle(input::KeyEvent{input::Key::a, false}));
    CHECK_FALSE(f.focus.handle(input::KeyEvent{input::Key::tab, false, true}));
    CHECK(f.focused() == 2);
}

TEST_CASE("an empty order uses no Tab") {
    widgets::FocusManager focus;
    CHECK_FALSE(focus.handle(tab));
}

TEST_CASE("the focus follows a click on a field, since the fields keep their own focus") {
    Fields f(3);
    const render::Rect b = f.list[2]->bounds();
    f.list[2]->handle({PointerAction::down, b.x + 10.0, b.y + 10.0, 1});
    CHECK(f.focused() == 2);
    CHECK(f.focus.handle(tab));
    CHECK(f.focused() == 0);
}

TEST_CASE("Tab with a single focusable keeps its focus without restarting the caret blink") {
    Fields f(1);
    f.list[0]->set_focused(true);
    f.list[0]->step(0.2);
    const auto before = f.focus.next_wake_seconds();
    REQUIRE(before);
    CHECK(f.focus.handle(tab));
    CHECK(f.focused() == 0);
    CHECK(*f.focus.next_wake_seconds() == doctest::Approx(*before));
}

TEST_CASE("the caret area and the next wake come from the focused text input") {
    Fields f(2);
    CHECK_FALSE(f.focus.caret_area());
    CHECK_FALSE(f.focus.next_wake_seconds());
    f.list[1]->set_focused(true);
    f.list[1]->step(0.1);
    const render::Rect expected = f.list[1]->caret_area();
    const auto area = f.focus.caret_area();
    REQUIRE(area);
    CHECK(area->x == expected.x);
    CHECK(area->y == expected.y);
    CHECK(area->height == expected.height);
    CHECK(*f.focus.next_wake_seconds() == doctest::Approx(f.list[1]->next_blink_seconds()));
}

TEST_CASE("a focusable without a caret takes part in Tab order but gives no caret area or wake") {
    Fields f(1);
    Toggle toggle;
    f.focus.add(toggle);
    toggle.set_focused(true);
    CHECK_FALSE(f.focus.caret_area());
    CHECK_FALSE(f.focus.next_wake_seconds());
    CHECK(f.focus.handle(tab));
    CHECK(f.focused() == 0);
    CHECK_FALSE(toggle.focused());
}

TEST_CASE("a removed widget leaves the Tab order and clear_focus drops the focus") {
    Fields f(3);
    f.focus.remove(*f.list[1]);
    CHECK(f.focus.size() == 2);
    f.list[0]->set_focused(true);
    CHECK(f.focus.handle(tab));
    CHECK(f.focused() == 2);
    f.focus.clear_focus();
    CHECK(f.focused() == -1);
}

TEST_CASE("Tab skips widgets that cannot take the focus now") {
    Fields f(2);
    Gated gated;
    f.focus.remove(*f.list[1]);
    f.focus.add(gated);
    f.focus.add(*f.list[1]);
    gated.enabled = false;
    f.list[0]->set_focused(true);
    CHECK(f.focus.handle(tab));
    CHECK(f.focused() == 1);
    CHECK_FALSE(gated.focused());
    CHECK(f.focus.handle(shift_tab));
    CHECK(f.focused() == 0);
    gated.enabled = true;
    CHECK(f.focus.handle(tab));
    CHECK(gated.focused());
    CHECK(f.focused() == -1);
}

TEST_CASE("Tab uses no key when no widget can take the focus") {
    widgets::FocusManager focus;
    Gated gated;
    gated.enabled = false;
    focus.add(gated);
    CHECK_FALSE(focus.handle(tab));
    CHECK_FALSE(gated.focused());
}
