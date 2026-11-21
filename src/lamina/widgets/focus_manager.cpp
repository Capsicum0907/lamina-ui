#include "lamina/widgets/focus_manager.hpp"

#include <algorithm>

namespace lamina::widgets {

bool FocusManager::handle(const input::KeyEvent& e) {
    if (e.key != input::Key::tab) {
        return false;
    }
    const Member* focused = focused_member();
    const bool tab_is_text = focused && focused->accepts_tab();
    if (e.ctrl ? !tab_is_text : tab_is_text && !e.shift) {
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
    if (current != entries_.end() && (*current)->advance_within(direction)) {
        return true;
    }
    std::size_t i = current == entries_.end() ? (forward ? n - 1 : 0) : static_cast<std::size_t>(current - entries_.begin());
    for (std::size_t tries = 0; tries < n; ++tries) {
        i = forward ? (i + 1) % n : (i + n - 1) % n;
        if (!entries_[i]->can_focus()) {
            continue;
        }
        if (current != entries_.end() && current->get() != entries_[i].get()) {
            (*current)->set_focused(false);
        }
        entries_[i]->enter(direction);
        return true;
    }
    return false;
}

bool FocusManager::advance(FocusDirection direction) {
    const std::size_t n = entries_.size();
    const auto current = std::find_if(entries_.begin(), entries_.end(), [](const auto& m) { return m->focused(); });
    const bool forward = direction == FocusDirection::next;
    const auto at = static_cast<std::ptrdiff_t>(current - entries_.begin());
    if (current != entries_.end() && (*current)->advance_within(direction)) {
        return true;
    }
    std::ptrdiff_t i = current == entries_.end() ? (forward ? -1 : static_cast<std::ptrdiff_t>(n)) : at;
    for (i += forward ? 1 : -1; i >= 0 && i < static_cast<std::ptrdiff_t>(n); i += forward ? 1 : -1) {
        Member& m = *entries_[static_cast<std::size_t>(i)];
        if (!m.can_focus()) {
            continue;
        }
        if (current != entries_.end()) {
            (*current)->set_focused(false);
        }
        m.enter(direction);
        return true;
    }
    return false;
}

std::optional<render::Rect> FocusManager::focused_bounds() const {
    const Member* m = focused_member();
    return m ? m->bounds() : std::nullopt;
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

bool FocusManager::claims_key(const input::KeyEvent& e) const {
    const Member* m = focused_member();
    return m && m->claims_key(e);
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

bool chain_focus(const std::vector<FocusManager*>& parts, FocusDirection direction, bool wrap) {
    const auto n = static_cast<std::ptrdiff_t>(parts.size());
    const bool forward = direction == FocusDirection::next;
    std::ptrdiff_t at = -1;
    for (std::ptrdiff_t i = 0; i < n; ++i) {
        if (parts[static_cast<std::size_t>(i)]->has_focus()) {
            at = i;
            break;
        }
    }
    if (at < 0) {
        for (std::ptrdiff_t k = 0; k < n; ++k) {
            FocusManager& part = *parts[static_cast<std::size_t>(forward ? k : n - 1 - k)];
            if (part.any_can_focus()) {
                return part.advance(direction);
            }
        }
        return false;
    }
    FocusManager& current = *parts[static_cast<std::size_t>(at)];
    if (current.advance(direction)) {
        return true;
    }
    for (std::ptrdiff_t k = 1; k <= n; ++k) {
        std::ptrdiff_t j = forward ? at + k : at - k;
        if (j < 0 || j >= n) {
            if (!wrap) {
                return false;
            }
            j = (j % n + n) % n;
        }
        FocusManager& part = *parts[static_cast<std::size_t>(j)];
        if (!part.any_can_focus()) {
            continue;
        }
        current.clear_focus();
        part.clear_focus();
        return part.advance(direction);
    }
    return true;
}

}
