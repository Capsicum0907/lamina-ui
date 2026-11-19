#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/widgets/protocol.hpp"

namespace lamina::widgets {

class WidgetSet {
public:
    template <Paintable W>
    W& add(W& widget) {
        entries_.push_back(std::make_unique<Entry<W&>>(widget));
        return widget;
    }

    template <Paintable W, typename... Args>
    W& emplace(Args&&... args) {
        auto entry = std::make_unique<Entry<W>>(std::forward<Args>(args)...);
        W& widget = entry->widget;
        entries_.push_back(std::move(entry));
        return widget;
    }

    void step(double dt_seconds) {
        for (auto& e : entries_) {
            e->step(dt_seconds);
        }
    }

    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const {
        for (const auto& e : entries_) {
            e->paint(canvas, scheme);
        }
    }

    bool animating() const {
        return std::any_of(entries_.begin(), entries_.end(), [](const auto& e) { return e->animating(); });
    }

    std::size_t size() const { return entries_.size(); }
    bool empty() const { return entries_.empty(); }

private:
    struct Member {
        virtual ~Member() = default;
        virtual void step(double dt_seconds) = 0;
        virtual void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const = 0;
        virtual bool animating() const = 0;
    };

    template <typename Held>
    struct Entry final : Member {
        template <typename... Args>
        explicit Entry(Args&&... args) : widget(std::forward<Args>(args)...) {}
        using Widget = std::remove_reference_t<Held>;
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
        Held widget;
    };

    std::vector<std::unique_ptr<Member>> entries_;
};

}
