#include "lamina/text/edit_history.hpp"

#include <utility>

namespace lamina::text {

bool EditHistory::merges(const EditState& before, EditGroup group) const {
    if (done_.empty() || group == EditGroup::single || idle_seconds_ > limits_.pause_seconds) {
        return false;
    }
    const Step& top = done_.back();
    return top.open && top.group == group && top.after.anchor == before.anchor && top.after.cursor == before.cursor;
}

void EditHistory::record(EditState before, EditState after, EditGroup group, bool starts_step) {
    if (limits_.depth == 0) {
        return;
    }
    undone_.clear();
    if (!starts_step && merges(before, group)) {
        done_.back().after = std::move(after);
    } else {
        seal();
        done_.push_back({std::move(before), std::move(after), group, group != EditGroup::single});
        if (done_.size() > limits_.depth) {
            done_.erase(done_.begin());
        }
    }
    idle_seconds_ = 0.0;
}

void EditHistory::seal() {
    if (!done_.empty()) {
        done_.back().open = false;
    }
}

void EditHistory::advance(double dt_seconds) {
    idle_seconds_ += dt_seconds;
}

void EditHistory::clear() {
    done_.clear();
    undone_.clear();
    idle_seconds_ = 0.0;
}

std::optional<EditState> EditHistory::undo() {
    if (done_.empty()) {
        return std::nullopt;
    }
    Step step = std::move(done_.back());
    done_.pop_back();
    step.open = false;
    EditState state = step.before;
    undone_.push_back(std::move(step));
    return state;
}

std::optional<EditState> EditHistory::redo() {
    if (undone_.empty()) {
        return std::nullopt;
    }
    Step step = std::move(undone_.back());
    undone_.pop_back();
    EditState state = step.after;
    done_.push_back(std::move(step));
    return state;
}

}
