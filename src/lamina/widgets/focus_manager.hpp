#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "lamina/input/events.hpp"
#include "lamina/render/geometry.hpp"
#include "lamina/widgets/protocol.hpp"

namespace lamina::widgets {

template <typename W>
concept TextInput = Focusable<W> && requires(const W& w) {
    { w.caret_area() } -> std::convertible_to<render::Rect>;
    { w.next_blink_seconds() } -> std::convertible_to<double>;
};

template <typename W>
concept KeyClaiming = Focusable<W> && requires(const W& w, const input::KeyEvent& e) {
    { w.claims_key(e) } -> std::convertible_to<bool>;
};

template <typename W>
concept TabAccepting = Focusable<W> && requires(const W& w) {
    { w.accepts_tab() } -> std::convertible_to<bool>;
};

enum class FocusDirection { next, previous };

class FocusManager;

bool chain_focus(const std::vector<FocusManager*>& parts, FocusDirection direction, bool wrap);

class FocusManager {
public:
    template <Focusable W>
    void add(W& widget) {
        entries_.push_back(std::make_unique<Entry<W>>(widget));
    }

    void add_group(FocusManager& group, std::function<bool()> shown = {}) {
        entries_.push_back(std::make_unique<Group>(group, std::move(shown)));
    }

    template <Focusable W>
    void remove(const W& widget) {
        remove_id(&widget);
    }

    void clear() { entries_.clear(); }

    bool handle(const input::KeyEvent& e);
    bool move_focus(FocusDirection direction);
    bool advance(FocusDirection direction);
    bool has_focus() const { return focused_member() != nullptr; }
    bool any_can_focus() const {
        return std::any_of(entries_.begin(), entries_.end(), [](const auto& m) { return m->can_focus(); });
    }
    std::optional<render::Rect> focused_bounds() const;
    void clear_focus();
    void suspend();
    void resume();
    void discard();
    std::size_t size() const { return entries_.size(); }

    std::optional<render::Rect> caret_area() const;
    std::optional<double> next_wake_seconds() const;
    bool claims_key(const input::KeyEvent& e) const;

private:
    struct Member {
        virtual ~Member() = default;
        virtual const void* id() const = 0;
        virtual bool focused() const = 0;
        virtual bool can_focus() const = 0;
        virtual bool accepts_tab() const = 0;
        virtual void set_focused(bool focused) = 0;
        virtual std::optional<render::Rect> caret_area() const = 0;
        virtual std::optional<double> next_wake_seconds() const = 0;
        virtual std::optional<render::Rect> bounds() const = 0;
        virtual bool claims_key(const input::KeyEvent&) const { return false; }
        virtual bool advance_within(FocusDirection) { return false; }
        virtual void enter(FocusDirection) { set_focused(true); }
    };

    struct Group final : Member {
        Group(FocusManager& g, std::function<bool()> s) : group(g), shown(std::move(s)) {}
        const void* id() const override { return &group; }
        bool focused() const override { return group.has_focus(); }
        bool can_focus() const override { return (!shown || shown()) && group.any_can_focus(); }
        bool accepts_tab() const override {
            const Member* m = group.focused_member();
            return m && m->accepts_tab();
        }
        void set_focused(bool focused) override {
            if (focused) {
                enter(FocusDirection::next);
            } else {
                group.clear_focus();
            }
        }
        std::optional<render::Rect> caret_area() const override { return group.caret_area(); }
        std::optional<double> next_wake_seconds() const override { return group.next_wake_seconds(); }
        std::optional<render::Rect> bounds() const override { return group.focused_bounds(); }
        bool claims_key(const input::KeyEvent& e) const override { return group.claims_key(e); }
        bool advance_within(FocusDirection direction) override { return group.advance(direction); }
        void enter(FocusDirection direction) override {
            group.clear_focus();
            group.advance(direction);
        }
        FocusManager& group;
        std::function<bool()> shown;
    };

    template <typename W>
    struct Entry final : Member {
        explicit Entry(W& w) : widget(w) {}
        const void* id() const override { return &widget; }
        bool focused() const override { return widget.focused(); }
        bool can_focus() const override {
            if constexpr (FocusGated<W>) {
                return widget.can_focus();
            } else {
                return true;
            }
        }
        bool accepts_tab() const override {
            if constexpr (TabAccepting<W>) {
                return widget.accepts_tab();
            } else {
                return false;
            }
        }
        void set_focused(bool focused) override { widget.set_focused(focused); }
        std::optional<render::Rect> caret_area() const override {
            if constexpr (TextInput<W>) {
                return widget.caret_area();
            } else {
                return std::nullopt;
            }
        }
        std::optional<double> next_wake_seconds() const override {
            if constexpr (TextInput<W>) {
                const double seconds = widget.next_blink_seconds();
                return seconds < 0.0 ? std::nullopt : std::optional<double>(seconds);
            } else {
                return std::nullopt;
            }
        }
        std::optional<render::Rect> bounds() const override {
            if constexpr (requires(const W& w) { { w.bounds() } -> std::convertible_to<render::Rect>; }) {
                return widget.bounds();
            } else {
                return std::nullopt;
            }
        }
        bool claims_key(const input::KeyEvent& e) const override {
            if constexpr (KeyClaiming<W>) {
                return widget.claims_key(e);
            } else {
                return false;
            }
        }
        W& widget;
    };

    void remove_id(const void* id);
    const Member* focused_member() const;
    Member* member(const void* id) const;

    std::vector<std::unique_ptr<Member>> entries_;
    std::vector<const void*> suspended_;
};

}
