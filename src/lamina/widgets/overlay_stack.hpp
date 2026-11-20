#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "lamina/input/events.hpp"
#include "lamina/widgets/focus_manager.hpp"
#include "lamina/widgets/protocol.hpp"

namespace lamina::widgets {

template <typename W>
concept Overlay = HandlesPointer<W> && HandlesKeys<W> && Cancellable<W> && requires(const W& w) {
    { w.is_open() } -> std::convertible_to<bool>;
};

enum class Modality { modal, modeless };

class OverlayStack {
public:
    template <Cancellable... Ws>
    void add_base(Ws&... base) {
        (base_.push_back([&base] { base.cancel_interaction(); }), ...);
    }

    void set_focus(FocusManager& focus) { focus_ = &focus; }

    template <typename W>
    void remove(const W& overlay) {
        for (auto& layer : layers_) {
            if (layer->id() == static_cast<const void*>(&overlay)) {
                layer->removed = true;
            }
        }
        if (routing_ == 0) {
            prune();
        }
    }

    template <Overlay W, typename OnResult>
    void push(W& overlay, OnResult on_result, Modality modality = Modality::modal) {
        bool holds_focus = false;
        for (auto& layer : layers_) {
            if (layer->id() == &overlay) {
                layer->stale = true;
                holds_focus = holds_focus || layer->holds_focus;
                layer->holds_focus = false;
            }
        }
        if (modality == Modality::modal) {
            cancel_below();
            if (focus_ && !holds_focus) {
                focus_->suspend();
                holds_focus = true;
            }
        }
        auto entry = std::make_unique<Entry<W, OnResult>>(overlay, std::move(on_result), modality);
        entry->holds_focus = holds_focus;
        layers_.push_back(std::move(entry));
    }

    bool handle(const input::PointerEvent& e);
    bool handle(const input::KeyEvent& e);
    bool handle(const input::TextEvent& e);
    bool handle(const input::WheelEvent& e);
    bool empty() const;

private:
    struct Layer {
        explicit Layer(Modality m) : modality(m) {}
        virtual ~Layer() = default;
        virtual const void* id() const = 0;
        virtual bool open() const = 0;
        virtual void cancel_interaction() = 0;
        virtual bool deliver(const input::PointerEvent& e) = 0;
        virtual bool deliver(const input::KeyEvent& e) = 0;
        virtual bool deliver(const input::TextEvent& e) = 0;
        virtual bool deliver(const input::WheelEvent& e) = 0;
        bool live() const { return !stale && !removed && open(); }
        Modality modality;
        bool stale = false;
        bool holds_focus = false;
        bool removed = false;
    };

    template <typename W, typename OnResult>
    struct Entry final : Layer {
        Entry(W& w, OnResult f, Modality m) : Layer(m), widget(w), on_result(std::move(f)) {}
        const void* id() const override { return &widget; }
        bool open() const override { return widget.is_open(); }
        void cancel_interaction() override { widget.cancel_interaction(); }
        bool deliver(const input::PointerEvent& e) override { return forward(e); }
        bool deliver(const input::KeyEvent& e) override { return forward(e); }
        bool deliver(const input::TextEvent& e) override {
            if constexpr (HandlesText<W>) {
                return forward(e);
            } else {
                return false;
            }
        }
        bool deliver(const input::WheelEvent& e) override {
            if constexpr (requires { widget.handle(e); }) {
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
                if constexpr (std::invocable<OnResult&, const Result&>) {
                    on_result(result);
                }
                return reports_consumed(result);
            }
        }

        W& widget;
        OnResult on_result;
    };

    template <typename E>
    bool route(const E& e);
    void prune(bool restore_focus = true);
    void cancel_below();

    std::vector<std::unique_ptr<Layer>> layers_;
    std::vector<std::function<void()>> base_;
    FocusManager* focus_ = nullptr;
    int routing_ = 0;
};

}
