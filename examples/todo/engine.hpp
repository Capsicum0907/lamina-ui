#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lamina::examples::todo {

using ItemId = std::uint64_t;

struct Item {
    ItemId id;
    std::string text;
    bool done;
    std::uint64_t created;
};

struct Format {
    std::string_view open_marker = "[ ]";
    std::string_view done_marker = "[x]";
    std::string_view done_marker_upper = "[X]";
    std::string_view separator = " ";
    std::string_view replacement = "\xEF\xBF\xBD";
};

enum class LineProblem { no_marker, empty_text, invalid_utf8 };

struct LoadReport {
    std::vector<std::pair<std::size_t, LineProblem>> problems;
};

enum class UndoKind { none, remove, clear };

std::string clean_text(std::string_view text, const Format& format = {});

class TodoList {
public:
    explicit TodoList(Format format = {});

    const std::vector<Item>& items() const { return items_; }
    const Item* find(ItemId id) const;
    std::optional<std::size_t> index_of(ItemId id) const;
    std::vector<ItemId> active() const;
    std::vector<ItemId> completed() const;
    std::size_t completed_count() const;
    std::uint64_t revision() const { return revision_; }

    std::optional<ItemId> add(std::string_view text);
    bool edit(ItemId id, std::string_view text);
    bool toggle(ItemId id);
    bool remove(ItemId id);
    bool move(ItemId id, std::size_t index);
    std::size_t clear_completed();

    UndoKind undo_kind() const;
    std::size_t undo_size() const { return undo_.size(); }
    std::vector<ItemId> undo();

    std::string save() const;
    static TodoList load(std::string_view text, LoadReport* report = nullptr, Format format = {});

private:
    Item make(std::string text, bool done);
    void changed() { ++revision_; }

    Format format_;
    std::vector<Item> items_;
    std::vector<std::pair<std::size_t, Item>> undo_;
    UndoKind undo_kind_ = UndoKind::none;
    ItemId next_id_ = 1;
    std::uint64_t next_created_ = 0;
    std::uint64_t revision_ = 0;
};

}
