#pragma once

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "lamina/input/events.hpp"

namespace lamina::input {

class TouchReleaseGate {
public:
    std::vector<PointerEvent> pointer(const PointerEvent& event) {
        std::vector<PointerEvent> out;
        if (event.kind != PointerKind::touch) {
            out.push_back(event);
            return out;
        }
        if (event.action == PointerAction::up) {
            const Ending ending = std::exchange(ended_, Ending::none);
            if (ending == Ending::cancelled) {
                out.push_back(leave_at(event));
            } else if (ending == Ending::lifted) {
                lift(event, out);
            } else {
                release(out);
                held_ = event;
                age_ = 0;
            }
            return out;
        }
        if (event.action == PointerAction::down) {
            release(out);
            ended_ = Ending::none;
        }
        out.push_back(event);
        return out;
    }

    std::vector<PointerEvent> finger(const FingerEvent& event) {
        std::vector<PointerEvent> out;
        if (event.action == FingerAction::down) {
            if (!tracked_) {
                tracked_ = event.finger;
            }
            return out;
        }
        if (event.action == FingerAction::move || tracked_ != event.finger) {
            return out;
        }
        tracked_.reset();
        const bool cancelled = event.action == FingerAction::cancel;
        if (!held_) {
            ended_ = cancelled ? Ending::cancelled : Ending::lifted;
            return out;
        }
        const PointerEvent up = *held_;
        held_.reset();
        if (cancelled) {
            out.push_back(leave_at(up));
        } else {
            lift(up, out);
        }
        return out;
    }

    std::vector<PointerEvent> end_frame() {
        std::vector<PointerEvent> out;
        if (held_ && age_ > 0) {
            release(out);
        } else if (held_) {
            ++age_;
        }
        return out;
    }

    bool holding() const { return held_.has_value(); }

private:
    enum class Ending { none, lifted, cancelled };

    static PointerEvent leave_at(const PointerEvent& up) {
        PointerEvent leave = up;
        leave.action = PointerAction::leave;
        leave.clicks = 0;
        return leave;
    }

    static void lift(const PointerEvent& up, std::vector<PointerEvent>& out) {
        out.push_back(up);
        out.push_back(leave_at(up));
    }

    void release(std::vector<PointerEvent>& out) {
        if (held_) {
            lift(*held_, out);
            held_.reset();
        }
    }

    std::optional<std::uint64_t> tracked_;
    std::optional<PointerEvent> held_;
    Ending ended_ = Ending::none;
    int age_ = 0;
};

}
