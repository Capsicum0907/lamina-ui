#pragma once

#include <cmath>

#include "lamina/input/events.hpp"

namespace lamina::input {

struct ScreenOrigin {
    double x_dp = 0.0;
    double y_dp = 0.0;
};

class TouchEchoFilter {
public:
    bool admit(const PointerEvent& event, ScreenOrigin origin, double tolerance_dp) {
        const double x = event.x_dp + origin.x_dp;
        const double y = event.y_dp + origin.y_dp;
        if (event.kind == PointerKind::touch) {
            if (event.action == PointerAction::down) {
                finger_down_ = true;
            } else if (event.action == PointerAction::up) {
                finger_down_ = false;
            }
            touched_ = true;
            touch_x_ = x;
            touch_y_ = y;
            return true;
        }
        if (event.kind != PointerKind::mouse) {
            return true;
        }
        if (event.action == PointerAction::down) {
            finger_down_ = false;
            return true;
        }
        if (event.action != PointerAction::move) {
            return true;
        }
        if (finger_down_) {
            return false;
        }
        return !(touched_ && std::abs(x - touch_x_) < tolerance_dp && std::abs(y - touch_y_) < tolerance_dp);
    }

private:
    bool finger_down_ = false;
    bool touched_ = false;
    double touch_x_ = 0.0;
    double touch_y_ = 0.0;
};

}
