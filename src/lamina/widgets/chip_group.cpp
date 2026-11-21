#include "lamina/widgets/chip_group.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "lamina/color/mix.hpp"
#include "lamina/widgets/item_focus.hpp"

namespace lamina::widgets {

namespace {

constexpr const tokens::ChipMotion& m = tokens::chip_motion;

}

ChipGroup::ChipGroup(ChipGroupOptions options, const text::FontStack& fonts) : options_(std::move(options)), fonts_(fonts) {
    for (ChipOptions& chip : options_.chips) {
        const bool allowed = chip.enabled;
        entries_.push_back({std::make_unique<Chip>(std::move(chip), fonts_), motion::Animated(1.0, m.group_presence),
                            motion::Animated(0.0, m.group_move), motion::Animated(0.0, m.group_move)});
        entries_.back().allowed = allowed;
    }
    options_.chips.clear();
    if (options_.single_selection) {
        bool seen = false;
        for (Entry& e : entries_) {
            if (e.chip->selected()) {
                if (seen) {
                    e.chip->set_selected(false);
                }
                seen = true;
            }
        }
    }
}

std::vector<std::size_t> ChipGroup::active() const {
    std::vector<std::size_t> out;
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        if (!entries_[i].leaving) {
            out.push_back(i);
        }
    }
    return out;
}

std::size_t ChipGroup::entry_index(std::size_t index) const {
    const std::vector<std::size_t> a = active();
    if (index >= a.size()) {
        throw std::out_of_range("no chip at that index");
    }
    return a[index];
}

std::size_t ChipGroup::size() const {
    return active().size();
}

Chip& ChipGroup::chip(std::size_t index) {
    return *entries_[entry_index(index)].chip;
}

const Chip& ChipGroup::chip(std::size_t index) const {
    return *entries_[entry_index(index)].chip;
}

std::optional<std::size_t> ChipGroup::find(std::string_view label) const {
    const std::vector<std::size_t> a = active();
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (entries_[a[i]].chip->label() == label) {
            return i;
        }
    }
    return std::nullopt;
}

std::vector<std::size_t> ChipGroup::selected() const {
    std::vector<std::size_t> out;
    const std::vector<std::size_t> a = active();
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (entries_[a[i]].chip->selected()) {
            out.push_back(i);
        }
    }
    return out;
}

void ChipGroup::add(ChipOptions options) {
    const bool allowed = options.enabled;
    options.enabled = allowed && enabled_;
    entries_.push_back({std::make_unique<Chip>(std::move(options), fonts_), motion::Animated(0.0, m.group_presence),
                        motion::Animated(0.0, m.group_move), motion::Animated(0.0, m.group_move)});
    entries_.back().allowed = allowed;
    entries_.back().presence.animate_to(1.0);
    if (options_.single_selection && entries_.back().chip->selected()) {
        select_only(entries_.size() - 1);
    }
}

void ChipGroup::start_leaving(std::size_t entry, ChipGroupResponse& response) {
    const std::vector<std::size_t> a = active();
    const auto pos = std::find(a.begin(), a.end(), entry);
    if (pos == a.end()) {
        return;
    }
    response.removed = static_cast<std::size_t>(pos - a.begin());
    response.removed_label = entries_[entry].chip->label();
    response.consumed = true;
    const bool had_focus = focused_ && focus_entry_ == entry;
    entries_[entry].leaving = true;
    entries_[entry].presence.animate_to(0.0);
    entries_[entry].chip->cancel_interaction();
    entries_[entry].chip->set_focused(false);
    if (had_focus) {
        const std::vector<std::size_t> rest = active();
        std::optional<std::size_t> next;
        for (std::size_t i : rest) {
            if (i > entry && entries_[i].chip->enabled()) {
                next = i;
                break;
            }
        }
        if (!next) {
            for (auto it = rest.rbegin(); it != rest.rend(); ++it) {
                if (entries_[*it].chip->enabled()) {
                    next = *it;
                    break;
                }
            }
        }
        focus_entry(next);
        if (!next) {
            focused_ = false;
        }
    }
}

void ChipGroup::remove(std::size_t index) {
    ChipGroupResponse ignored;
    start_leaving(entry_index(index), ignored);
}

double ChipGroup::flow(double max_width, bool place) {
    double x = 0.0;
    double y = 0.0;
    double line_height = 0.0;
    double widest = 0.0;
    std::size_t line = 0;
    bool line_started = false;
    for (Entry& e : entries_) {
        const double k = std::clamp(e.presence.value(), 0.0, 1.0);
        const double w = e.chip->width() * k;
        const double gap = line_started ? options_.spacing * k : 0.0;
        if (line_started && x + gap + w > max_width + 1e-6 && k > 0.0) {
            y += line_height + options_.line_spacing;
            x = 0.0;
            line_height = 0.0;
            ++line;
            line_started = false;
        }
        const double start = x + (line_started ? gap : 0.0);
        if (place) {
            const render::Point at{start, y};
            if (e.placed && e.line != line && !e.leaving) {
                e.dx.snap_to(e.dx.value() + e.at.x - at.x);
                e.dy.snap_to(e.dy.value() + e.at.y - at.y);
                e.dx.animate_to(0.0);
                e.dy.animate_to(0.0);
            }
            e.at = at;
            e.line = line;
            e.placed = true;
        }
        x = start + w;
        line_height = std::max(line_height, e.chip->height());
        widest = std::max(widest, x);
        line_started = true;
    }
    if (place) {
        line_count_ = entries_.empty() ? 0 : line + 1;
        width_ = widest;
    }
    return entries_.empty() ? 0.0 : y + line_height;
}

layout::Size ChipGroup::measure(const layout::Constraints& c) {
    const double max_width = std::isfinite(c.max_width) ? c.max_width : layout::unbounded;
    double widest = 0.0;
    const double h = flow(max_width, false);
    for (const Entry& e : entries_) {
        widest = std::max(widest, e.chip->width());
    }
    return c.constrain({std::isfinite(max_width) ? max_width : widest, h});
}

void ChipGroup::arrange(const render::Rect& r) {
    origin_ = {r.x, r.y};
    laid_out_width_ = r.width;
    height_ = flow(r.width, true);
    width_ = r.width;
    for (Entry& e : entries_) {
        e.chip->set_origin({origin_.x + e.at.x + e.dx.value(), origin_.y + e.at.y + e.dy.value()});
    }
}

void ChipGroup::select_only(std::size_t entry) {
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        if (i != entry) {
            entries_[i].chip->set_selected(false);
        }
    }
}

void ChipGroup::focus_entry(std::optional<std::size_t> entry) {
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        entries_[i].chip->set_focused(focused_ && entry && i == *entry);
    }
    if (entry) {
        focus_entry_ = *entry;
    }
}

std::optional<std::size_t> ChipGroup::focused_chip() const {
    if (!focused_) {
        return std::nullopt;
    }
    const std::vector<std::size_t> a = active();
    const auto pos = std::find(a.begin(), a.end(), focus_entry_);
    return pos == a.end() ? std::nullopt : std::optional<std::size_t>(static_cast<std::size_t>(pos - a.begin()));
}

bool ChipGroup::can_focus() const {
    return enabled_ && std::any_of(entries_.begin(), entries_.end(), [](const Entry& e) { return !e.leaving && e.chip->enabled(); });
}

void ChipGroup::set_focused(bool focused) {
    focused = focused && can_focus();
    if (focused && !focused_) {
        const std::vector<std::size_t> a = active();
        std::optional<std::size_t> start;
        const auto current = std::find(a.begin(), a.end(), focus_entry_);
        if (current != a.end() && entries_[*current].chip->enabled()) {
            start = *current;
        }
        for (std::size_t i : a) {
            if (!start && entries_[i].chip->enabled()) {
                start = i;
            }
        }
        focused_ = true;
        focus_entry(start);
        return;
    }
    focused_ = focused;
    focus_entry(focused ? std::optional<std::size_t>(focus_entry_) : std::nullopt);
}

void ChipGroup::set_enabled(bool enabled) {
    enabled_ = enabled;
    for (Entry& e : entries_) {
        e.chip->set_enabled(enabled && e.allowed);
    }
    if (!can_focus()) {
        set_focused(false);
    }
}

void ChipGroup::set_density(tokens::Density density) {
    density_ = density;
    for (Entry& e : entries_) {
        e.chip->set_density(density);
    }
}

void ChipGroup::set_chip_enabled(std::size_t index, bool enabled) {
    Entry& e = entries_[entry_index(index)];
    e.allowed = enabled;
    e.chip->set_enabled(enabled_ && enabled);
    if (focused_ && focus_entry_ == entry_index(index) && !enabled) {
        set_focused(false);
        set_focused(true);
    }
}

AccessibleNode ChipGroup::accessible() const {
    AccessibleNode node{AccessibleRole::group, options_.accessible_name, bounds(), {.enabled = enabled_, .focusable = can_focus(), .focused = focused_}};
    for (std::size_t i : active()) {
        node.children.push_back(entries_[i].chip->accessible());
    }
    return node;
}

void ChipGroup::after_click(std::size_t entry, ChipGroupResponse& response) {
    const std::vector<std::size_t> a = active();
    const auto pos = std::find(a.begin(), a.end(), entry);
    response.clicked = static_cast<std::size_t>(pos - a.begin());
    response.consumed = true;
    if (options_.single_selection && entries_[entry].chip->selected()) {
        select_only(entry);
    }
}

ChipGroupResponse ChipGroup::handle(const input::PointerEvent& e) {
    if (e.action == input::PointerAction::down) {
        focused_ = false;
    }
    ChipGroupResponse response;
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].leaving) {
            continue;
        }
        const ChipResponse r = entries_[i].chip->handle(e);
        if (r.removed) {
            start_leaving(i, response);
        } else if (r.clicked) {
            after_click(i, response);
        }
    }
    return response;
}

ChipGroupResponse ChipGroup::handle(const input::KeyEvent& e) {
    ChipGroupResponse response;
    if (!focused_) {
        return response;
    }
    const std::vector<std::size_t> a = active();
    const auto pos = std::find(a.begin(), a.end(), focus_entry_);
    if (pos == a.end()) {
        return response;
    }
    const std::size_t current = static_cast<std::size_t>(pos - a.begin());
    if (const std::optional<std::size_t> target =
            item_key_target(e, current, a.size(), ItemAxis::horizontal, [&](std::size_t i) { return entries_[a[i]].chip->enabled(); })) {
        focus_entry(a[*target]);
        response.consumed = true;
        return response;
    }
    const ChipResponse r = entries_[focus_entry_].chip->handle(e);
    if (r.removed) {
        start_leaving(focus_entry_, response);
    } else if (r.clicked) {
        after_click(focus_entry_, response);
    }
    return response;
}

void ChipGroup::cancel_interaction() {
    for (Entry& e : entries_) {
        e.chip->cancel_interaction();
    }
}

void ChipGroup::step(double dt) {
    for (Entry& e : entries_) {
        e.chip->step(dt);
        e.presence.step(dt);
        e.dx.step(dt);
        e.dy.step(dt);
    }
    std::erase_if(entries_, [](const Entry& e) { return e.leaving && e.presence.settled() && e.presence.target() <= 0.0; });
    if (laid_out_width_ > 0.0) {
        arrange({origin_.x, origin_.y, laid_out_width_, height_});
    }
}

bool ChipGroup::animating() const {
    return std::any_of(entries_.begin(), entries_.end(), [](const Entry& e) {
        return e.chip->animating() || !e.presence.settled() || !e.dx.settled() || !e.dy.settled();
    });
}

void ChipGroup::paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
    for (const Entry& e : entries_) {
        const double k = std::clamp(e.presence.value(), 0.0, 1.0);
        if (k <= 0.0) {
            continue;
        }
        if (k >= 1.0) {
            e.chip->paint(canvas, scheme);
            continue;
        }
        const render::Rect b = e.chip->bounds();
        const double margin = tokens::chip_tokens.group_clip_margin;
        canvas.push_clip(render::Rect{b.x - margin, b.y - margin, b.width * k + margin, b.height + 2.0 * margin});
        e.chip->paint(canvas, color::faded(scheme, k));
        canvas.pop_clip();
    }
}

}
