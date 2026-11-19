#include <doctest/doctest.h>

#include <string>

#include "todo/engine.hpp"

using namespace lamina::examples::todo;

namespace {

std::vector<std::string> texts(const TodoList& list) {
    std::vector<std::string> out;
    for (const Item& i : list.items()) {
        out.push_back(i.text);
    }
    return out;
}

TodoList three() {
    TodoList list;
    list.add("c");
    list.add("b");
    list.add("a");
    return list;
}

}

TEST_CASE("add puts a new item at the top with a fresh id and creation order") {
    TodoList list;
    const auto first = list.add("Buy milk");
    const auto second = list.add("Call mom");
    REQUIRE(first);
    REQUIRE(second);
    CHECK(*first != *second);
    CHECK(texts(list) == std::vector<std::string>{"Call mom", "Buy milk"});
    CHECK(list.find(*second)->created > list.find(*first)->created);
    CHECK_FALSE(list.find(*first)->done);
}

TEST_CASE("add cleans the text and refuses an empty item") {
    TodoList list;
    CHECK_FALSE(list.add(""));
    CHECK_FALSE(list.add("   \t "));
    CHECK(list.items().empty());
    CHECK(list.revision() == 0);
    const auto id = list.add("  two\nlines\tand tab  ");
    REQUIRE(id);
    CHECK(list.find(*id)->text == "two lines and tab");
}

TEST_CASE("clean_text replaces invalid UTF-8 and keeps valid multibyte text") {
    CHECK(clean_text("牛乳を買う") == "牛乳を買う");
    CHECK(clean_text("a\xFF" "b") == "a\xEF\xBF\xBD" "b");
    CHECK(clean_text("\xC0\xAF") == "\xEF\xBF\xBD\xEF\xBF\xBD");
    CHECK(clean_text("\xED\xA0\x80") == "\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD");
    CHECK(clean_text("\xE3\x81") == "\xEF\xBF\xBD\xEF\xBF\xBD");
    CHECK(clean_text("🍅") == "🍅");
}

TEST_CASE("edit changes the text, but not to an empty one") {
    TodoList list;
    const ItemId id = *list.add("draft");
    const auto before = list.revision();
    CHECK(list.edit(id, "final"));
    CHECK(list.find(id)->text == "final");
    CHECK(list.revision() == before + 1);
    CHECK_FALSE(list.edit(id, "  "));
    CHECK_FALSE(list.edit(id, "final"));
    CHECK(list.find(id)->text == "final");
    CHECK_FALSE(list.edit(id + 100, "x"));
}

TEST_CASE("toggle marks an item done and back, keeping its place in the order") {
    TodoList list = three();
    const ItemId b = list.items()[1].id;
    CHECK(list.toggle(b));
    CHECK(list.find(b)->done);
    CHECK(list.active().size() == 2);
    CHECK(list.completed() == std::vector<ItemId>{b});
    CHECK(list.completed_count() == 1);
    CHECK(texts(list) == std::vector<std::string>{"a", "b", "c"});
    CHECK(list.toggle(b));
    CHECK_FALSE(list.find(b)->done);
    CHECK(list.completed().empty());
}

TEST_CASE("remove deletes one item and undo puts it back where it was") {
    TodoList list = three();
    const ItemId b = list.items()[1].id;
    CHECK(list.undo_kind() == UndoKind::none);
    CHECK(list.remove(b));
    CHECK(texts(list) == std::vector<std::string>{"a", "c"});
    CHECK(list.undo_kind() == UndoKind::remove);
    CHECK(list.undo_size() == 1);
    CHECK(list.undo() == std::vector<ItemId>{b});
    CHECK(texts(list) == std::vector<std::string>{"a", "b", "c"});
    CHECK(list.find(b));
    CHECK(list.undo_kind() == UndoKind::none);
    CHECK(list.undo().empty());
    CHECK_FALSE(list.remove(12345));
}

TEST_CASE("only the last delete or clear can be undone") {
    TodoList list = three();
    const ItemId a = list.items()[0].id;
    const ItemId c = list.items()[2].id;
    list.remove(a);
    list.remove(c);
    CHECK(list.undo() == std::vector<ItemId>{c});
    CHECK(texts(list) == std::vector<std::string>{"b", "c"});
    CHECK(list.undo().empty());
}

TEST_CASE("undo after the list changed puts the item back at its old index") {
    TodoList list = three();
    list.remove(list.items()[1].id);
    list.add("new");
    list.undo();
    CHECK(texts(list) == std::vector<std::string>{"new", "b", "a", "c"});
}

TEST_CASE("clear_completed removes every done item and undo restores them in place") {
    TodoList list;
    for (const char* t : {"e", "d", "c", "b", "a"}) {
        list.add(t);
    }
    list.toggle(list.items()[1].id);
    list.toggle(list.items()[3].id);
    CHECK(list.clear_completed() == 2);
    CHECK(texts(list) == std::vector<std::string>{"a", "c", "e"});
    CHECK(list.undo_kind() == UndoKind::clear);
    CHECK(list.undo_size() == 2);
    CHECK(list.undo().size() == 2);
    CHECK(texts(list) == std::vector<std::string>{"a", "b", "c", "d", "e"});
    CHECK(list.completed_count() == 2);
    TodoList none = three();
    const auto before = none.revision();
    CHECK(none.clear_completed() == 0);
    CHECK(none.undo_kind() == UndoKind::none);
    CHECK(none.revision() == before);
}

TEST_CASE("move reorders an item to an index, clamped to the end") {
    TodoList list = three();
    const ItemId a = list.items()[0].id;
    CHECK(list.move(a, 2));
    CHECK(texts(list) == std::vector<std::string>{"b", "c", "a"});
    CHECK(list.move(a, 0));
    CHECK(texts(list) == std::vector<std::string>{"a", "b", "c"});
    CHECK(list.move(a, 99));
    CHECK(texts(list) == std::vector<std::string>{"b", "c", "a"});
    CHECK_FALSE(list.move(a, 2));
    CHECK_FALSE(list.move(999, 0));
}

TEST_CASE("every change counts up the revision") {
    TodoList list;
    const ItemId id = *list.add("x");
    CHECK(list.revision() == 1);
    list.toggle(id);
    list.remove(id);
    list.undo();
    CHECK(list.revision() == 4);
}

TEST_CASE("save writes one line per item with its done marker") {
    TodoList list = three();
    list.toggle(list.items()[1].id);
    CHECK(list.save() == "[ ] a\n[x] b\n[ ] c\n");
    CHECK(TodoList{}.save().empty());
}

TEST_CASE("load reads what save wrote") {
    const std::string text = "[ ] Buy milk\n[x] 牛乳を買う\n[ ] A long task that will surely wrap across several lines\n[x] 🍅 tomatoes\n";
    LoadReport report;
    const TodoList list = TodoList::load(text, &report);
    CHECK(report.problems.empty());
    REQUIRE(list.items().size() == 4);
    CHECK(list.items()[1].text == "牛乳を買う");
    CHECK(list.items()[1].done);
    CHECK_FALSE(list.items()[2].done);
    CHECK(list.items()[0].created > list.items()[3].created);
    CHECK(list.save() == text);
    CHECK(TodoList::load(list.save()).save() == text);
}

TEST_CASE("load survives malformed lines and reports them") {
    const std::string text =
        "\xEF\xBB\xBF[ ] first\r\n"
        "\r\n"
        "no marker here\n"
        "[x]\n"
        "[X] upper done\n"
        "[ ]tight\n"
        "[ ] bad \xFF byte\n"
        "   \n"
        "[x] last without newline";
    LoadReport report;
    const TodoList list = TodoList::load(text, &report);
    std::vector<std::string> got;
    for (const Item& i : list.items()) {
        got.push_back(i.text);
    }
    CHECK(got == std::vector<std::string>{"first", "no marker here", "upper done", "tight", "bad \xEF\xBF\xBD byte", "last without newline"});
    CHECK_FALSE(list.items()[1].done);
    CHECK(list.items()[2].done);
    CHECK(list.items()[5].done);
    using P = std::pair<std::size_t, LineProblem>;
    CHECK(report.problems == std::vector<P>{{3, LineProblem::no_marker}, {4, LineProblem::empty_text}, {7, LineProblem::invalid_utf8}});
    CHECK(list.save() == "[ ] first\n[ ] no marker here\n[x] upper done\n[ ] tight\n[ ] bad \xEF\xBF\xBD byte\n[x] last without newline\n");
    CHECK(TodoList::load("").items().empty());
    CHECK(TodoList::load("\n\n\n").items().empty());
}

TEST_CASE("ids stay unique after load and later adds") {
    TodoList list = TodoList::load("[ ] a\n[ ] b\n");
    const ItemId added = *list.add("c");
    CHECK(added != list.items()[1].id);
    CHECK(added != list.items()[2].id);
    CHECK(list.items()[0].created > list.items()[1].created);
}
