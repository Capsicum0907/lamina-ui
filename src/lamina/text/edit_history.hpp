#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace lamina::text {

struct EditHistoryLimits {
    std::size_t depth = 0;
    double pause_seconds = 0.0;
};

struct EditState {
    std::u32string text;
    std::size_t anchor = 0;
    std::size_t cursor = 0;
};

enum class EditGroup { typing, deleting, single };

class EditHistory {
public:
    explicit EditHistory(EditHistoryLimits limits = {}) : limits_(limits) {}

    void record(EditState before, EditState after, EditGroup group, bool starts_step = false);
    void seal();
    void advance(double dt_seconds);
    void clear();

    bool can_undo() const { return !done_.empty(); }
    bool can_redo() const { return !undone_.empty(); }
    std::size_t undo_depth() const { return done_.size(); }
    std::optional<EditState> undo();
    std::optional<EditState> redo();

private:
    struct Step {
        EditState before;
        EditState after;
        EditGroup group;
        bool open;
    };

    bool merges(const EditState& before, EditGroup group) const;

    EditHistoryLimits limits_;
    std::vector<Step> done_;
    std::vector<Step> undone_;
    double idle_seconds_ = 0.0;
};

}
