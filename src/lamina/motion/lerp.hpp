#pragma once

namespace lamina::motion {

constexpr double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

}
