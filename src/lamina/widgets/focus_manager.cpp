#include "lamina/widgets/focus_manager.hpp"

#include <algorithm>

namespace lamina::widgets {

bool FocusManager::handle(const input::KeyEvent& e) {
    if (e.key != input::Key::tab || e.ctrl) {
        return false;
    }
    return move_focus(e.shift ? FocusDirection::previous : FocusDirection::next);
}

bool FocusManager::move_focus(FocusDirection direction) {
    const std::size_t n = entries_.size();
    if (n == 0) {
        return false;
    }
    const auto current = std::find_if(entries_.begin(), entries_.end(), [](const auto& m) { return m->focused(); });
    const bool forward = direction == FocusDirection::next;
    std::size_t i = current == entries_.end() ? (forward ? n - 1 : 0) : static_cast<std::size_t>(current - entries_.begin());
    for (std::size_t tries = 0; tries < n; ++tries) {
        i = forward ? (i + 1) % n : (i + n - 1) % n;
        if (!entries_[i]->can_focus()) {
            continue;
        }
        if (current != entries_.end() && current->get() != entries_[i].get()) {
            (*current)->set_focused(false);
        }
        entries_[i]->set_focused(true);
        return true;
    }
    return false;
}

void FocusManager::clear_focus() {
    for (auto& m : entries_) {
        m->set_focused(false);
    }
}

void FocusManager::suspend() {
    const Member* m = focused_member();
    suspended_.push_back(m ? m->id() : nullptr);
    clear_focus();
}

void FocusManager::resume() {
    if (suspended_.empty()) {
        return;
    }
    const void* id = suspended_.back();
    suspended_.pop_back();
    Member* m = member(id);
    if (!m || !m->can_focus()) {
        return;
    }
    clear_focus();
    m->set_focused(true);
}

void FocusManager::discard() {
    if (!suspended_.empty()) {
        suspended_.pop_back();
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

FocusManager::Member* FocusManager::member(const void* id) const {
    if (!id) {
        return nullptr;
    }
    const auto it = std::find_if(entries_.begin(), entries_.end(), [id](const auto& m) { return m->id() == id; });
    return it == entries_.end() ? nullptr : it->get();
}

}
