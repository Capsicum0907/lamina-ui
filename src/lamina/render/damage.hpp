#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "lamina/render/geometry.hpp"

namespace lamina::render {

struct PixelRect {
    int x;
    int y;
    int width;
    int height;

    bool operator==(const PixelRect&) const = default;
    bool empty() const { return width <= 0 || height <= 0; }
    std::int64_t area() const { return empty() ? 0 : static_cast<std::int64_t>(width) * height; }
};

PixelRect pixel_union(const PixelRect& a, const PixelRect& b);

class Fingerprint {
public:
    Fingerprint& add(std::uint64_t value);
    Fingerprint& add(double value);
    Fingerprint& add(Point p);
    Fingerprint& add_bytes(std::span<const std::uint8_t> bytes);
    std::uint64_t value() const { return hash_; }

    static std::uint64_t combine(std::uint64_t hash, std::uint64_t value);

private:
    std::uint64_t hash_ = seed;
    static constexpr std::uint64_t seed = 0x6a09e667f3bcc909ull;
};

struct DamageSettings {
    int tile;
    std::size_t rects_max;
    std::size_t merge_input_max;
    double full_fraction;
    bool whole_rows = false;
};

std::vector<PixelRect> merge_tiles(const std::vector<std::uint8_t>& dirty, int columns, int rows, int tile, int width, int height,
                                   std::size_t rects_max, std::size_t merge_input_max, bool whole_rows = false);

class DamageGrid {
public:
    struct Result {
        std::vector<PixelRect> rects;
        std::size_t tiles = 0;
        bool full = false;
    };

    void begin(int width, int height, int tile);
    bool begun() const { return begun_; }
    void add(const Rect& device, std::uint64_t hash);
    Result commit(const DamageSettings& settings);
    void invalidate();

    int columns() const { return columns_; }
    int rows() const { return rows_; }

private:
    Result full_result();

    int width_ = 0;
    int height_ = 0;
    int tile_ = 0;
    int columns_ = 0;
    int rows_ = 0;
    bool begun_ = false;
    bool valid_ = false;
    std::vector<std::uint64_t> current_;
    std::vector<std::uint64_t> previous_;
};

}
