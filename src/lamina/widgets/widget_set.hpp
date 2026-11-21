#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/input/events.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina/widgets/protocol.hpp"

namespace lamina::widgets {

struct NoResultHandler {};

class WidgetSet {
public:
    template <Paintable W>
    W& add(W& widget) {
        return add(widget, NoResultHandler{});
    }

    template <Paintable W, typename OnResult>
    W& add(W& widget, OnResult on_result) {
        entries_.push_back(std::make_unique<Entry<W&, OnResult>>(std::move(on_result), widget));
        return widget;
    }

    template <Paintable W, typename... Args>
    W& emplace(Args&&... args) {
        return emplace_with<W>(NoResultHandler{}, std::forward<Args>(args)...);
    }

    template <Paintable W, typename OnResult, typename... Args>
    W& emplace_with(OnResult on_result, Args&&... args) {
        auto entry = std::make_unique<Entry<W, OnResult>>(std::move(on_result), std::forward<Args>(args)...);
        W& widget = entry->widget;
        entries_.push_back(std::move(entry));
        return widget;
    }

    template <typename W>
    bool remove(const W& widget) {
        bool found = false;
        for (auto& e : entries_) {
            if (!e->removed && e->address() == static_cast<const void*>(&widget)) {
                e->removed = true;
                found = true;
            }
        }
        sweep();
        return found;
    }

    template <typename W>
    bool contains(const W& widget) const {
        return std::any_of(entries_.begin(), entries_.end(),
                           [&](const auto& e) { return !e->removed && e->address() == static_cast<const void*>(&widget); });
    }

    template <typename W, typename F>
    void for_each(F&& f) {
        visit([&](Member& m) {
            if (void* w = m.as(type_tag<W>())) {
                f(*static_cast<W*>(w));
            }
        });
    }

    template <typename W, typename F>
    void for_each(F&& f) const {
        visit([&](const Member& m) {
            if (const void* w = m.as(type_tag<W>())) {
                f(*static_cast<const W*>(w));
            }
        });
    }

    bool handle(const input::PointerEvent& e) { return deliver(e); }
    bool handle(const input::KeyEvent& e) { return deliver(e); }
    bool handle(const input::TextEvent& e) { return deliver(e); }

    void step(double dt_seconds) {
        visit([&](Member& m) { m.step(dt_seconds); });
    }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
        visit([&](const Member& m) { m.paint(canvas, scheme); });
    }

    void cancel_interaction() {
        visit([](Member& m) { m.cancel_interaction(); });
    }

    void set_enabled(bool enabled) {
        visit([enabled](Member& m) { m.set_enabled(enabled); });
    }

    void set_density(tokens::Density density) {
        density_ = density;
        visit([density](Member& m) { m.set_density(density); });
    }
    tokens::Density density() const { return density_; }

    std::vector<AccessibleNode> accessible_nodes() const {
        std::vector<AccessibleNode> out;
        visit([&](const Member& m) { m.collect(out); });
        return out;
    }

    bool animating() const {
        return std::any_of(entries_.begin(), entries_.end(), [](const auto& e) { return !e->removed && e->animating(); });
    }

    std::size_t size() const {
        return static_cast<std::size_t>(std::count_if(entries_.begin(), entries_.end(), [](const auto& e) { return !e->removed; }));
    }
    bool empty() const { return size() == 0; }

private:
    using TypeTag = const void*;

    template <typename W>
    static TypeTag type_tag() {
        static const char tag = 0;
        return &tag;
    }

    struct Member {
        virtual ~Member() = default;
        virtual const void* address() const = 0;
        virtual void* as(TypeTag tag) = 0;
        virtual const void* as(TypeTag tag) const = 0;
        virtual void step(double dt_seconds) = 0;
        virtual void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const = 0;
        virtual bool animating() const = 0;
        virtual void cancel_interaction() = 0;
        virtual void set_enabled(bool enabled) = 0;
        virtual void set_density(tokens::Density density) = 0;
        virtual void collect(std::vector<AccessibleNode>& out) const = 0;
        virtual bool deliver(const input::PointerEvent& e) = 0;
        virtual bool deliver(const input::KeyEvent& e) = 0;
        virtual bool deliver(const input::TextEvent& e) = 0;
        bool removed = false;
    };

    template <typename Held, typename OnResult>
    struct Entry final : Member {
        using Widget = std::remove_reference_t<Held>;

        template <typename... Args>
        explicit Entry(OnResult f, Args&&... args) : widget(std::forward<Args>(args)...), on_result(std::move(f)) {}

        const void* address() const override { return &widget; }
        void* as(TypeTag tag) override { return tag == type_tag<Widget>() ? static_cast<void*>(&widget) : nullptr; }
        const void* as(TypeTag tag) const override { return tag == type_tag<Widget>() ? static_cast<const void*>(&widget) : nullptr; }
        void step(double dt_seconds) override {
            if constexpr (Steppable<Widget>) {
                widget.step(dt_seconds);
            }
        }
        void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const override { widget.paint(canvas, scheme); }
        bool animating() const override {
            if constexpr (Steppable<Widget>) {
                return widget.animating();
            } else {
                return false;
            }
        }
        void cancel_interaction() override {
            if constexpr (Cancellable<Widget>) {
                widget.cancel_interaction();
            }
        }
        void set_enabled(bool enabled) override {
            if constexpr (Disableable<Widget>) {
                widget.set_enabled(enabled);
            }
        }
        void set_density(tokens::Density density) override {
            if constexpr (Densifiable<Widget>) {
                widget.set_density(density);
            }
        }
        void collect(std::vector<AccessibleNode>& out) const override {
            if constexpr (Accessible<Widget>) {
                std::optional<AccessibleNode> node = widget.accessible();
                if (node) {
                    out.push_back(std::move(*node));
                }
            }
        }
        bool deliver(const input::PointerEvent& e) override {
            if constexpr (HandlesPointer<Widget>) {
                return forward(e);
            } else {
                return false;
            }
        }
        bool deliver(const input::KeyEvent& e) override {
            if constexpr (HandlesKeys<Widget>) {
                return forward(e);
            } else {
                return false;
            }
        }
        bool deliver(const input::TextEvent& e) override {
            if constexpr (HandlesText<Widget>) {
                return forward(e);
            } else {
                return false;
            }
        }

        template <typename E>
        bool forward(const E& e) {
            using Result = decltype(widget.handle(e));
            if constexpr (std::is_void_v<Result>) {
                widget.handle(e);
                return false;
            } else {
                const Result result = widget.handle(e);
                if constexpr (std::invocable<OnResult&, Widget&, const Result&>) {
                    on_result(widget, result);
                } else if constexpr (std::invocable<OnResult&, const Result&>) {
                    on_result(result);
                }
                return reports_consumed(result);
            }
        }

        Held widget;
        OnResult on_result;
    };

    template <typename F>
    void visit(F&& f) {
        ++depth_;
        const std::size_t n = entries_.size();
        for (std::size_t i = 0; i < n; ++i) {
            Member& m = *entries_[i];
            if (!m.removed) {
                f(m);
            }
        }
        --depth_;
        sweep();
    }

    template <typename F>
    void visit(F&& f) const {
        ++depth_;
        const std::size_t n = entries_.size();
        for (std::size_t i = 0; i < n; ++i) {
            const Member& m = *entries_[i];
            if (!m.removed) {
                f(m);
            }
        }
        --depth_;
    }

    template <typename E>
    bool deliver(const E& e) {
        bool consumed = false;
        visit([&](Member& m) { consumed = m.deliver(e) || consumed; });
        return consumed;
    }

    void sweep() {
        if (depth_ == 0) {
            std::erase_if(entries_, [](const auto& e) { return e->removed; });
        }
    }

    std::vector<std::unique_ptr<Member>> entries_;
    mutable int depth_ = 0;
    tokens::Density density_{};
};

}
