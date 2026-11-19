#include "lamina/widgets/focus_manager.hpp"

#include <algorithm>

namespace lamina::widgets {

bool FocusManager::handle(const input::KeyEvent& e) {
    if (e.key != input::Key::tab) {
        return false;
    }
    return move_focus(e.shift ? FocusDirection::previous : FocusDirection::next);
}

bool FocusManager::move_focus(FocusDirection direction) {
    const auto current = std::find_if(entries_.begin(), entries_.end(), [](const auto& m) { return m->focused(); });
    if (current == entries_.end()) {
        return false;
    }
    const std::size_t n = entries_.size();
    const std::size_t i = static_cast<std::size_t>(current - entries_.begin());
    const std::size_t target = direction == FocusDirection::next ? (i + 1) % n : (i + n - 1) % n;
    if (target != i) {
        entries_[i]->set_focused(false);
        entries_[target]->set_focused(true);
    }
    return true;
}

void FocusManager::clear_focus() {
    for (auto& m : entries_) {
        m->set_focused(false);
    }
}

std::optional<render::Rect> FocusManager::caret_area() const {
    const Member* m = focused_member();
    return m ? m->caret_area() : std::nullopt;
}

std::optional<double> FocusManager::next_wake_seconds() const {
    const Member* m = focused_member();
    return m ? m->next_wake_seconds() : std::nullopt;
}

void FocusManager::remove_id(const void* id) {
    std::erase_if(entries_, [id](const auto& m) { return m->id() == id; });
}

const FocusManager::Member* FocusManager::focused_member() const {
    const auto it = std::find_if(entries_.begin(), entries_.end(), [](const auto& m) { return m->focused(); });
    return it == entries_.end() ? nullptr : it->get();
}

}
