#pragma once

#include <concepts>
#include <cstddef>
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

enum class FocusDirection { next, previous };

class FocusManager {
public:
    template <Focusable W>
    void add(W& widget) {
        entries_.push_back(std::make_unique<Entry<W>>(widget));
    }

    template <Focusable W>
    void remove(const W& widget) {
        remove_id(&widget);
    }

    void clear() { entries_.clear(); }

    bool handle(const input::KeyEvent& e);
    bool move_focus(FocusDirection direction);
    void clear_focus();
    void suspend();
    void resume();
    std::size_t size() const { return entries_.size(); }

    std::optional<render::Rect> caret_area() const;
    std::optional<double> next_wake_seconds() const;

private:
    struct Member {
        virtual ~Member() = default;
        virtual const void* id() const = 0;
        virtual bool focused() const = 0;
        virtual bool can_focus() const = 0;
        virtual void set_focused(bool focused) = 0;
        virtual std::optional<render::Rect> caret_area() const = 0;
        virtual std::optional<double> next_wake_seconds() const = 0;
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
        W& widget;
    };

    void remove_id(const void* id);
    const Member* focused_member() const;
    Member* member(const void* id) const;

    std::vector<std::unique_ptr<Member>> entries_;
    std::vector<const void*> suspended_;
};

}
