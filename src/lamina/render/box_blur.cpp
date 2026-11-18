#include "lamina/render/box_blur.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace lamina::render {

namespace {

constexpr int box_passes = 3;

void box_pass(const std::uint32_t* src, std::uint32_t* dst, int count, int step, int radius) {
    const int window = 2 * radius + 1;
    std::uint32_t sum[4] = {0, 0, 0, 0};
    auto at = [&](int i) { return src[std::clamp(i, 0, count - 1) * step]; };
    auto add = [&](std::uint32_t px, int sign) {
        for (int c = 0; c < 4; ++c) {
            sum[c] += static_cast<std::uint32_t>(sign * static_cast<int>((px >> (c * 8)) & 0xff));
        }
    };
    for (int i = -radius; i <= radius; ++i) {
        add(at(i), 1);
    }
    for (int i = 0; i < count; ++i) {
        std::uint32_t out = 0;
        for (int c = 0; c < 4; ++c) {
            out |= ((sum[c] + window / 2) / window) << (c * 8);
        }
        dst[i * step] = out;
        add(at(i + radius + 1), 1);
        add(at(i - radius), -1);
    }
}

}

void gaussian_blur_prgb32(std::uint8_t* pixels, int width, int height, std::intptr_t stride, double sigma) {
    if (sigma <= 0.0 || width <= 0 || height <= 0) {
        return;
    }
    const double ideal = std::sqrt(12.0 * sigma * sigma / box_passes + 1.0);
    const int radius = std::max(1, static_cast<int>(std::lround((ideal - 1.0) / 2.0)));

    std::vector<std::uint32_t> line(static_cast<std::size_t>(std::max(width, height)));
    std::vector<std::uint32_t> tmp(line.size());
    for (int y = 0; y < height; ++y) {
        auto* row = reinterpret_cast<std::uint32_t*>(pixels + stride * y);
        std::copy(row, row + width, line.begin());
        for (int p = 0; p < box_passes; ++p) {
            box_pass(line.data(), tmp.data(), width, 1, radius);
            line.swap(tmp);
        }
        std::copy(line.begin(), line.begin() + width, row);
    }
    const std::intptr_t step = stride / 4;
    for (int x = 0; x < width; ++x) {
        auto* col = reinterpret_cast<std::uint32_t*>(pixels) + x;
        for (int y = 0; y < height; ++y) {
            line[y] = col[y * step];
        }
        for (int p = 0; p < box_passes; ++p) {
            box_pass(line.data(), tmp.data(), height, 1, radius);
            line.swap(tmp);
        }
        for (int y = 0; y < height; ++y) {
            col[y * step] = line[y];
        }
    }
}

}
