#include "todo/engine.hpp"

#include <algorithm>

namespace lamina::examples::todo {

namespace {

constexpr std::string_view byte_order_mark = "\xEF\xBB\xBF";

std::size_t sequence_length(std::string_view s, std::size_t i) {
    const auto byte = [&](std::size_t k) { return static_cast<unsigned char>(s[k]); };
    const unsigned char b = byte(i);
    if (b < 0x80) {
        return 1;
    }
    std::size_t n = 0;
    char32_t min = 0;
    char32_t c = 0;
    if ((b & 0xE0) == 0xC0) {
        n = 2;
        min = 0x80;
        c = b & 0x1F;
    } else if ((b & 0xF0) == 0xE0) {
        n = 3;
        min = 0x800;
        c = b & 0x0F;
    } else if ((b & 0xF8) == 0xF0) {
        n = 4;
        min = 0x10000;
        c = b & 0x07;
    } else {
        return 0;
    }
    if (i + n > s.size()) {
        return 0;
    }
    for (std::size_t k = 1; k < n; ++k) {
        if ((byte(i + k) & 0xC0) != 0x80) {
            return 0;
        }
        c = (c << 6) | (byte(i + k) & 0x3F);
    }
    if (c < min || c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) {
        return 0;
    }
    return n;
}

bool valid_utf8(std::string_view s) {
    for (std::size_t i = 0; i < s.size();) {
        const std::size_t n = sequence_length(s, i);
        if (n == 0) {
            return false;
        }
        i += n;
    }
    return true;
}

bool is_space(char c) {
    return c == ' ';
}

std::string_view trim(std::string_view s) {
    while (!s.empty() && is_space(s.front())) {
        s.remove_prefix(1);
    }
    while (!s.empty() && is_space(s.back())) {
        s.remove_suffix(1);
    }
    return s;
}

}

std::string clean_text(std::string_view text, const Format& format) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size();) {
        const std::size_t n = sequence_length(text, i);
        if (n == 0) {
            out += format.replacement;
            ++i;
            continue;
        }
        if (n == 1 && (static_cast<unsigned char>(text[i]) < 0x20 || text[i] == 0x7F)) {
            out += ' ';
        } else {
            out.append(text.substr(i, n));
        }
        i += n;
    }
    return std::string(trim(out));
}

TodoList::TodoList(Format format) : format_(format) {}

Item TodoList::make(std::string text, bool done) {
    return {next_id_++, std::move(text), done, next_created_++};
}

const Item* TodoList::find(ItemId id) const {
    const auto it = std::find_if(items_.begin(), items_.end(), [id](const Item& i) { return i.id == id; });
    return it == items_.end() ? nullptr : &*it;
}

std::optional<std::size_t> TodoList::index_of(ItemId id) const {
    const auto it = std::find_if(items_.begin(), items_.end(), [id](const Item& i) { return i.id == id; });
    if (it == items_.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(it - items_.begin());
}

std::vector<ItemId> TodoList::active() const {
    std::vector<ItemId> out;
    for (const Item& i : items_) {
        if (!i.done) {
            out.push_back(i.id);
        }
    }
    return out;
}

std::vector<ItemId> TodoList::completed() const {
    std::vector<ItemId> out;
    for (const Item& i : items_) {
        if (i.done) {
            out.push_back(i.id);
        }
    }
    return out;
}

std::size_t TodoList::completed_count() const {
    return static_cast<std::size_t>(std::count_if(items_.begin(), items_.end(), [](const Item& i) { return i.done; }));
}

std::optional<ItemId> TodoList::add(std::string_view text) {
    std::string cleaned = clean_text(text, format_);
    if (cleaned.empty()) {
        return std::nullopt;
    }
    items_.insert(items_.begin(), make(std::move(cleaned), false));
    changed();
    return items_.front().id;
}

bool TodoList::edit(ItemId id, std::string_view text) {
    const auto i = index_of(id);
    std::string cleaned = clean_text(text, format_);
    if (!i || cleaned.empty() || items_[*i].text == cleaned) {
        return false;
    }
    items_[*i].text = std::move(cleaned);
    changed();
    return true;
}

bool TodoList::toggle(ItemId id) {
    const auto i = index_of(id);
    if (!i) {
        return false;
    }
    items_[*i].done = !items_[*i].done;
    changed();
    return true;
}

bool TodoList::remove(ItemId id) {
    const auto i = index_of(id);
    if (!i) {
        return false;
    }
    undo_ = {{*i, items_[*i]}};
    undo_kind_ = UndoKind::remove;
    items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(*i));
    changed();
    return true;
}

bool TodoList::move(ItemId id, std::size_t index) {
    const auto from = index_of(id);
    if (!from) {
        return false;
    }
    const std::size_t to = std::min(index, items_.size() - 1);
    if (to == *from) {
        return false;
    }
    Item item = std::move(items_[*from]);
    items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(*from));
    items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(to), std::move(item));
    changed();
    return true;
}

std::size_t TodoList::clear_completed() {
    std::vector<std::pair<std::size_t, Item>> removed;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (items_[i].done) {
            removed.emplace_back(i, items_[i]);
        }
    }
    if (removed.empty()) {
        return 0;
    }
    std::erase_if(items_, [](const Item& i) { return i.done; });
    undo_ = std::move(removed);
    undo_kind_ = UndoKind::clear;
    changed();
    return undo_.size();
}

UndoKind TodoList::undo_kind() const {
    return undo_.empty() ? UndoKind::none : undo_kind_;
}

std::vector<ItemId> TodoList::undo() {
    std::vector<ItemId> restored;
    for (auto& [index, item] : undo_) {
        const std::size_t at = std::min(index, items_.size());
        restored.push_back(item.id);
        items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(at), std::move(item));
    }
    undo_.clear();
    undo_kind_ = UndoKind::none;
    if (!restored.empty()) {
        changed();
    }
    return restored;
}

std::string TodoList::save() const {
    std::string out;
    for (const Item& i : items_) {
        out += i.done ? format_.done_marker : format_.open_marker;
        out += format_.separator;
        out += i.text;
        out += '\n';
    }
    return out;
}

TodoList TodoList::load(std::string_view text, LoadReport* report, Format format) {
    TodoList list(format);
    if (text.starts_with(byte_order_mark)) {
        text.remove_prefix(byte_order_mark.size());
    }
    auto note = [&](std::size_t line, LineProblem problem) {
        if (report) {
            report->problems.emplace_back(line, problem);
        }
    };
    std::size_t line_number = 0;
    while (!text.empty()) {
        const std::size_t end = text.find('\n');
        std::string_view line = text.substr(0, end);
        text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
        ++line_number;
        if (line.ends_with('\r')) {
            line.remove_suffix(1);
        }
        if (trim(line).empty()) {
            continue;
        }
        if (!valid_utf8(line)) {
            note(line_number, LineProblem::invalid_utf8);
        }
        bool done = false;
        std::string_view body = line;
        if (line.starts_with(format.open_marker)) {
            body.remove_prefix(format.open_marker.size());
        } else if (line.starts_with(format.done_marker) || line.starts_with(format.done_marker_upper)) {
            body.remove_prefix(format.done_marker.size());
            done = true;
        } else {
            note(line_number, LineProblem::no_marker);
        }
        std::string cleaned = clean_text(body, format);
        if (cleaned.empty()) {
            note(line_number, LineProblem::empty_text);
            continue;
        }
        list.items_.push_back(list.make(std::move(cleaned), done));
    }
    for (std::size_t i = 0; i < list.items_.size(); ++i) {
        list.items_[i].created = list.items_.size() - 1 - i;
    }
    return list;
}

}
