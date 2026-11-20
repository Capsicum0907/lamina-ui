#include <doctest/doctest.h>

#include <memory>
#include <string>
#include <vector>

#include "lamina/widgets/checkbox.hpp"
#include "lamina/widgets/dialog.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/label.hpp"
#include "lamina/widgets/nodes.hpp"
#include "lamina/widgets/text_field.hpp"
#include "lamina/widgets/widget_set.hpp"
#include "lamina_thorvg/canvas.hpp"
#include "support.hpp"

using namespace lamina;
using namespace lamina::test;
using input::Key;
using input::PointerAction;
using widgets::DialogAction;
using widgets::DialogResult;

namespace {

constexpr input::KeyEvent tab{Key::tab, false};
constexpr input::KeyEvent shift_tab{Key::tab, true};
constexpr layout::Size window{800.0, 600.0};

struct Content {
    widgets::WidgetSet widgets;
    widgets::FocusManager focus;
    std::vector<layout::NodePtr> nodes;

    layout::NodePtr column() {
        auto c = layout::column(nodes);
        c->spacing(8.0);
        return c;
    }
};

std::unique_ptr<Content> checkboxes(int count) {
    auto c = std::make_unique<Content>();
    for (int i = 0; i < count; ++i) {
        auto& box = c->widgets.emplace<widgets::Checkbox>();
        c->focus.add(box);
        c->nodes.push_back(widgets::node(box));
    }
    return c;
}

std::unique_ptr<Content> paragraphs(int count) {
    auto c = std::make_unique<Content>();
    for (int i = 0; i < count; ++i) {
        auto& label = c->widgets.emplace<widgets::Label>(widgets::LabelOptions{"Line " + std::to_string(i)}, fonts());
        c->nodes.push_back(widgets::node(label));
    }
    return c;
}

widgets::DialogOptions options(Content& content, std::string supporting = {}) {
    return {"Choose", std::move(supporting), "OK", "Cancel", std::nullopt, std::nullopt,
            widgets::DialogContent{content.column(), &content.widgets, &content.focus}};
}

DialogResult click(widgets::Dialog& d, double x, double y) {
    d.handle({PointerAction::down, x, y});
    return d.handle({PointerAction::up, x, y});
}

}

TEST_CASE("short content sits between the headline and the actions without scrolling") {
    auto content = checkboxes(2);
    widgets::Dialog d(options(*content), fonts());
    d.open(window);
    REQUIRE(d.content_viewport());
    const render::Rect v = *d.content_viewport();
    CHECK_FALSE(d.content_scrolls());
    CHECK(v.y == doctest::Approx(d.headline_bounds().y + d.headline_bounds().height + 16.0));
    CHECK(d.confirm_bounds().y == doctest::Approx(v.y + v.height + 24.0));
    CHECK(v.height == doctest::Approx(d.content_height()));
}

TEST_CASE("tall content is capped by the window and scrolls") {
    auto content = paragraphs(60);
    widgets::Dialog d(options(*content), fonts());
    d.open(window);
    CHECK(d.content_scrolls());
    CHECK(d.bounds().height == doctest::Approx(window.height - 2.0 * tokens::dialog_tokens.window_margin));
    const render::Rect v = *d.content_viewport();
    CHECK(v.height < d.content_height());
    CHECK(d.handle(input::WheelEvent{v.x + 10.0, v.y + 10.0, 0.0, 2.0}));
    settle(d);
    CHECK(d.content_scroll().offset() > 0.0);
    CHECK_FALSE(d.handle(input::WheelEvent{5.0, 5.0, 0.0, 2.0}));
}

TEST_CASE("the text-only dialog keeps its supporting text in the same slot") {
    auto content = checkboxes(1);
    widgets::Dialog d(options(*content, "Pick one of these."), fonts());
    d.open(window);
    const render::Rect v = *d.content_viewport();
    CHECK(d.content_height() > widgets::Checkbox().height() + 16.0);
    CHECK(d.confirm_bounds().y == doctest::Approx(v.y + v.height + 24.0));
}

TEST_CASE("a click in the content reaches the content's widgets in content coordinates") {
    auto content = checkboxes(2);
    widgets::Checkbox* second = nullptr;
    content->widgets.for_each<widgets::Checkbox>([&](widgets::Checkbox& c) { second = &c; });
    widgets::Dialog d(options(*content), fonts());
    d.open(window);
    const render::Rect v = *d.content_viewport();
    const render::Rect b = second->bounds();
    CHECK(click(d, v.x + b.x + b.width / 2.0, v.y + b.y + b.height / 2.0) == DialogResult::none);
    CHECK(second->state() == tokens::CheckboxState::on);
    CHECK(d.is_open());
}

TEST_CASE("Tab goes through the content first, then the actions, and wraps") {
    auto content = checkboxes(2);
    std::vector<widgets::Checkbox*> boxes;
    content->widgets.for_each<widgets::Checkbox>([&](widgets::Checkbox& c) { boxes.push_back(&c); });
    widgets::Dialog d(options(*content), fonts());
    d.open(window, input::InputSource::keyboard);
    CHECK(d.focused_action() == DialogAction::dismiss);
    d.handle(tab);
    CHECK(d.focused_action() == DialogAction::confirm);
    d.handle(tab);
    CHECK_FALSE(d.focused_action());
    CHECK(d.content_focused());
    CHECK(boxes[0]->focused());
    d.handle(tab);
    CHECK(boxes[1]->focused());
    CHECK(d.handle(input::KeyEvent{Key::space}) == DialogResult::none);
    CHECK(boxes[1]->state() == tokens::CheckboxState::on);
    CHECK(d.handle(input::KeyEvent{Key::enter}) == DialogResult::none);
    CHECK(d.is_open());
    d.handle(tab);
    CHECK_FALSE(d.content_focused());
    CHECK(d.focused_action() == DialogAction::dismiss);
    d.handle(shift_tab);
    CHECK(boxes[1]->focused());
    d.handle(shift_tab);
    d.handle(shift_tab);
    CHECK(d.focused_action() == DialogAction::confirm);
}

TEST_CASE("a disabled action is left out of the focus and the default") {
    auto content = checkboxes(1);
    widgets::DialogOptions o = options(*content);
    o.default_action = DialogAction::confirm;
    widgets::Dialog d(std::move(o), fonts());
    d.set_action_enabled(DialogAction::confirm, false);
    CHECK_FALSE(d.action_enabled(DialogAction::confirm));
    d.open(window, input::InputSource::keyboard);
    CHECK(d.focused_action() == DialogAction::dismiss);
    d.handle(tab);
    CHECK(d.content_focused());
    d.handle(tab);
    CHECK(d.focused_action() == DialogAction::dismiss);
    CHECK(click(d, d.confirm_bounds().x + 10.0, d.confirm_bounds().y + 10.0) == DialogResult::none);
    d.set_action_enabled(DialogAction::confirm, true);
    CHECK(click(d, d.confirm_bounds().x + 10.0, d.confirm_bounds().y + 10.0) == DialogResult::confirmed);
}

TEST_CASE("a text field in the content takes text and places the IME in window coordinates") {
    auto c = std::make_unique<Content>();
    auto& field = c->widgets.emplace<widgets::TextField>(widgets::TextFieldOptions{.label = "Name"}, fonts());
    c->focus.add(field);
    c->nodes.push_back(widgets::node(field));
    widgets::Dialog d(options(*c), fonts());
    d.open(window);
    const render::Rect v = *d.content_viewport();
    d.handle({PointerAction::down, v.x + 20.0, v.y + 20.0});
    d.handle({PointerAction::up, v.x + 20.0, v.y + 20.0});
    CHECK(field.focused());
    CHECK(d.content_focused());
    CHECK(d.handle(input::TextEvent{input::TextEventKind::commit, "Ada", 0}));
    CHECK(field.text() == "Ada");
    REQUIRE(d.caret_area());
    CHECK(d.caret_area()->x > v.x);
    CHECK(d.caret_area()->y >= v.y);
    CHECK(d.next_wake_seconds());
    d.handle({PointerAction::down, d.headline_bounds().x + 2.0, d.headline_bounds().y + 2.0});
    CHECK_FALSE(field.focused());
}

TEST_CASE("Tab into content that is scrolled away brings the focused widget into view") {
    auto c = std::make_unique<Content>();
    for (int i = 0; i < 40; ++i) {
        auto& label = c->widgets.emplace<widgets::Label>(widgets::LabelOptions{"Line " + std::to_string(i)}, fonts());
        c->nodes.push_back(widgets::node(label));
    }
    auto& box = c->widgets.emplace<widgets::Checkbox>();
    c->focus.add(box);
    c->nodes.push_back(widgets::node(box));
    widgets::Dialog d(options(*c), fonts());
    d.open(window, input::InputSource::keyboard);
    d.handle(tab);
    d.handle(tab);
    CHECK(box.focused());
    settle(d);
    const render::Rect v = *d.content_viewport();
    CHECK(box.bounds().y + box.bounds().height <= d.content_scroll().offset() + v.height + 0.5);
}

TEST_CASE("the content paints inside the dialog, clipped to its viewport") {
    auto content = paragraphs(60);
    widgets::Dialog d(options(*content), fonts());
    d.open(window);
    settle(d);
    thorvg::Canvas canvas(800, 600);
    canvas.clear(0xffffffff);
    d.paint(canvas, scheme());
    const render::Rect v = *d.content_viewport();
    int ink = 0;
    for (int y = static_cast<int>(v.y); y < static_cast<int>(v.y + v.height); ++y) {
        for (int x = static_cast<int>(v.x); x < static_cast<int>(v.x + 60.0); ++x) {
            ink += canvas.pixel(x, y) != canvas.pixel(static_cast<int>(v.x + v.width - 20.0), static_cast<int>(v.y)) ? 1 : 0;
        }
    }
    CHECK(ink > 0);
}
