#include "lamina/render/damage.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>

namespace lamina::render {

namespace {

constexpr std::uint64_t golden = 0x9e3779b97f4a7c15ull;
constexpr std::uint64_t mix_a = 0xbf58476d1ce4e5b9ull;
constexpr std::uint64_t mix_b = 0x94d049bb133111ebull;
constexpr int shift_a = 30;
constexpr int shift_b = 27;
constexpr int shift_c = 31;
constexpr int bytes_rotation = 29;

std::uint64_t finalize(std::uint64_t z) {
    z = (z ^ (z >> shift_a)) * mix_a;
    z = (z ^ (z >> shift_b)) * mix_b;
    return z ^ (z >> shift_c);
}

}

PixelRect pixel_union(const PixelRect& a, const PixelRect& b) {
    if (a.empty()) {
        return b;
    }
    if (b.empty()) {
        return a;
    }
    const int x0 = std::min(a.x, b.x);
    const int y0 = std::min(a.y, b.y);
    const int x1 = std::max(a.x + a.width, b.x + b.width);
    const int y1 = std::max(a.y + a.height, b.y + b.height);
    return {x0, y0, x1 - x0, y1 - y0};
}

std::uint64_t Fingerprint::combine(std::uint64_t hash, std::uint64_t value) {
    return finalize(hash + golden + finalize(value + golden));
}

Fingerprint& Fingerprint::add(std::uint64_t value) {
    hash_ = combine(hash_, value);
    return *this;
}

Fingerprint& Fingerprint::add(double value) {
    return add(std::bit_cast<std::uint64_t>(value));
}

Fingerprint& Fingerprint::add(Point p) {
    return add(p.x).add(p.y);
}

Fingerprint& Fingerprint::add_bytes(std::span<const std::uint8_t> bytes) {
    std::uint64_t h = hash_ ^ bytes.size();
    std::size_t i = 0;
    for (; i + sizeof(std::uint64_t) <= bytes.size(); i += sizeof(std::uint64_t)) {
        std::uint64_t word = 0;
        std::memcpy(&word, bytes.data() + i, sizeof(word));
        h = std::rotl(h ^ (word * mix_a), bytes_rotation) * mix_b;
    }
    std::uint64_t tail = 0;
    if (i < bytes.size()) {
        std::memcpy(&tail, bytes.data() + i, bytes.size() - i);
    }
    h = std::rotl(h ^ (tail * mix_a), bytes_rotation) * mix_b;
    return add(finalize(h));
}

std::vector<PixelRect> merge_tiles(const std::vector<std::uint8_t>& dirty, int columns, int rows, int tile, int width, int height,
                                   std::size_t rects_max, std::size_t merge_input_max, bool whole_rows) {
    struct Run {
        int c0;
        int c1;
        int r0;
        int r1;
    };
    std::vector<Run> done;
    std::vector<Run> open;
    std::vector<Run> next;
    for (int r = 0; r < rows; ++r) {
        next.clear();
        int c = 0;
        while (c < columns) {
            if (!dirty[static_cast<std::size_t>(r) * columns + c]) {
                c += 1;
                continue;
            }
            int c0 = c;
            while (c < columns && dirty[static_cast<std::size_t>(r) * columns + c]) {
                c += 1;
            }
            if (whole_rows) {
                c0 = 0;
                c = columns;
            }
            auto it = std::find_if(open.begin(), open.end(), [&](const Run& o) { return o.c0 == c0 && o.c1 == c; });
            if (it != open.end()) {
                Run grown = *it;
                grown.r1 = r + 1;
                open.erase(it);
                next.push_back(grown);
            } else {
                next.push_back({c0, c, r, r + 1});
            }
        }
        done.insert(done.end(), open.begin(), open.end());
        open.swap(next);
    }
    done.insert(done.end(), open.begin(), open.end());

    std::vector<PixelRect> rects;
    rects.reserve(done.size());
    for (const Run& run : done) {
        const int x0 = run.c0 * tile;
        const int y0 = run.r0 * tile;
        const int x1 = std::min(width, run.c1 * tile);
        const int y1 = std::min(height, run.r1 * tile);
        if (x1 > x0 && y1 > y0) {
            rects.push_back({x0, y0, x1 - x0, y1 - y0});
        }
    }
    if (rects.size() > merge_input_max) {
        PixelRect all{0, 0, 0, 0};
        for (const PixelRect& r : rects) {
            all = pixel_union(all, r);
        }
        return {all};
    }
    const std::size_t limit = std::max<std::size_t>(rects_max, 1);
    while (rects.size() > limit) {
        std::size_t best_i = 0;
        std::size_t best_j = 1;
        std::int64_t best_cost = std::numeric_limits<std::int64_t>::max();
        for (std::size_t i = 0; i < rects.size(); ++i) {
            for (std::size_t j = i + 1; j < rects.size(); ++j) {
                const std::int64_t cost = pixel_union(rects[i], rects[j]).area() - rects[i].area() - rects[j].area();
                if (cost < best_cost) {
                    best_cost = cost;
                    best_i = i;
                    best_j = j;
                }
            }
        }
        rects[best_i] = pixel_union(rects[best_i], rects[best_j]);
        rects.erase(rects.begin() + static_cast<std::ptrdiff_t>(best_j));
    }
    return rects;
}

void DamageGrid::begin(int width, int height, int tile) {
    const int t = std::max(1, tile);
    if (width != width_ || height != height_ || t != tile_) {
        width_ = width;
        height_ = height;
        tile_ = t;
        columns_ = width > 0 ? (width + t - 1) / t : 0;
        rows_ = height > 0 ? (height + t - 1) / t : 0;
        valid_ = false;
    }
    current_.assign(static_cast<std::size_t>(columns_) * static_cast<std::size_t>(rows_), Fingerprint{}.value());
    begun_ = true;
}

void DamageGrid::add(const Rect& device, std::uint64_t hash) {
    if (!begun_ || columns_ == 0 || rows_ == 0 || !(device.width > 0.0) || !(device.height > 0.0)) {
        return;
    }
    const double t = tile_;
    const int c0 = std::clamp(static_cast<int>(std::floor(device.x / t)), 0, columns_ - 1);
    const int r0 = std::clamp(static_cast<int>(std::floor(device.y / t)), 0, rows_ - 1);
    const int c1 = std::clamp(static_cast<int>(std::ceil((device.x + device.width) / t)), c0 + 1, columns_);
    const int r1 = std::clamp(static_cast<int>(std::ceil((device.y + device.height) / t)), r0 + 1, rows_);
    for (int r = r0; r < r1; ++r) {
        std::uint64_t* row = current_.data() + static_cast<std::size_t>(r) * columns_;
        for (int c = c0; c < c1; ++c) {
            row[c] = Fingerprint::combine(row[c], hash);
        }
    }
}

DamageGrid::Result DamageGrid::full_result() {
    Result out;
    out.full = true;
    out.tiles = static_cast<std::size_t>(columns_) * static_cast<std::size_t>(rows_);
    if (width_ > 0 && height_ > 0) {
        out.rects.push_back({0, 0, width_, height_});
    }
    return out;
}

DamageGrid::Result DamageGrid::commit(const DamageSettings& settings) {
    if (!begun_) {
        valid_ = false;
        return full_result();
    }
    if (!valid_) {
        previous_ = current_;
        valid_ = true;
        return full_result();
    }
    std::vector<std::uint8_t> dirty(current_.size(), 0);
    std::size_t tiles = 0;
    for (std::size_t i = 0; i < current_.size(); ++i) {
        if (current_[i] != previous_[i]) {
            dirty[i] = 1;
            tiles += 1;
        }
    }
    previous_ = current_;
    Result out;
    out.tiles = tiles;
    if (tiles == 0) {
        return out;
    }
    if (static_cast<double>(tiles) >= settings.full_fraction * static_cast<double>(current_.size())) {
        Result full = full_result();
        full.tiles = tiles;
        return full;
    }
    out.rects = merge_tiles(dirty, columns_, rows_, tile_, width_, height_, settings.rects_max, settings.merge_input_max, settings.whole_rows);
    return out;
}

void DamageGrid::invalidate() {
    valid_ = false;
    begun_ = false;
}

}
