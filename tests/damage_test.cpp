#include <doctest/doctest.h>

#include <cstdint>
#include <vector>

#include "lamina/render/damage.hpp"

using namespace lamina::render;

namespace {

constexpr DamageSettings settings{16, 4, 48, 0.6};

std::int64_t total_area(const std::vector<PixelRect>& rects) {
    std::int64_t a = 0;
    for (const PixelRect& r : rects) {
        a += r.area();
    }
    return a;
}

bool covers(const std::vector<PixelRect>& rects, int x, int y) {
    for (const PixelRect& r : rects) {
        if (x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height) {
            return true;
        }
    }
    return false;
}

}

TEST_CASE("a fingerprint depends on the values and their order") {
    Fingerprint a;
    a.add(1.0).add(2.0);
    Fingerprint b;
    b.add(1.0).add(2.0);
    Fingerprint c;
    c.add(2.0).add(1.0);
    CHECK(a.value() == b.value());
    CHECK(a.value() != c.value());
    const std::vector<std::uint8_t> bytes{1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<std::uint8_t> other = bytes;
    other.back() = 10;
    CHECK(Fingerprint{}.add_bytes(bytes).value() == Fingerprint{}.add_bytes(bytes).value());
    CHECK(Fingerprint{}.add_bytes(bytes).value() != Fingerprint{}.add_bytes(other).value());
    CHECK(Fingerprint{}.add_bytes({}).value() != Fingerprint{}.value());
}

TEST_CASE("the first commit of a damage grid repaints everything, and an unchanged frame repaints nothing") {
    DamageGrid grid;
    grid.begin(100, 50, 16);
    grid.add({10.0, 10.0, 20.0, 20.0}, 7);
    const DamageGrid::Result first = grid.commit(settings);
    CHECK(first.full);
    REQUIRE(first.rects.size() == 1);
    CHECK(first.rects[0] == PixelRect{0, 0, 100, 50});
    grid.begin(100, 50, 16);
    grid.add({10.0, 10.0, 20.0, 20.0}, 7);
    const DamageGrid::Result same = grid.commit(settings);
    CHECK_FALSE(same.full);
    CHECK(same.rects.empty());
    CHECK(same.tiles == 0);
}

TEST_CASE("a moved command damages the tiles it left and the tiles it entered") {
    DamageGrid grid;
    grid.begin(160, 160, 16);
    grid.add({8.0, 8.0, 10.0, 10.0}, 3);
    grid.commit(settings);
    grid.begin(160, 160, 16);
    grid.add({100.0, 100.0, 10.0, 10.0}, 3);
    const DamageGrid::Result moved = grid.commit(settings);
    CHECK_FALSE(moved.full);
    CHECK(covers(moved.rects, 10, 10));
    CHECK(covers(moved.rects, 105, 105));
    CHECK_FALSE(covers(moved.rects, 60, 60));
}

TEST_CASE("a changed hash in the same place damages its tiles, and order inside a tile counts") {
    DamageGrid grid;
    grid.begin(64, 64, 16);
    grid.add({0.0, 0.0, 64.0, 64.0}, 1);
    grid.add({20.0, 20.0, 4.0, 4.0}, 2);
    grid.add({40.0, 40.0, 4.0, 4.0}, 3);
    grid.commit(settings);
    grid.begin(64, 64, 16);
    grid.add({0.0, 0.0, 64.0, 64.0}, 1);
    grid.add({40.0, 40.0, 4.0, 4.0}, 3);
    grid.add({20.0, 20.0, 4.0, 4.0}, 2);
    const DamageGrid::Result swapped = grid.commit(settings);
    CHECK(swapped.tiles == 0);
    grid.begin(64, 64, 16);
    grid.add({20.0, 20.0, 4.0, 4.0}, 2);
    grid.add({0.0, 0.0, 64.0, 64.0}, 1);
    grid.add({40.0, 40.0, 4.0, 4.0}, 3);
    const DamageGrid::Result reordered = grid.commit(settings);
    CHECK(reordered.tiles == 1);
    CHECK(covers(reordered.rects, 21, 21));
}

TEST_CASE("a damage grid repaints everything after an invalidation or a size change") {
    DamageGrid grid;
    grid.begin(64, 64, 16);
    grid.commit(settings);
    grid.begin(64, 64, 16);
    grid.invalidate();
    CHECK_FALSE(grid.begun());
    CHECK(grid.commit(settings).full);
    grid.begin(64, 64, 16);
    CHECK(grid.commit(settings).full);
    grid.begin(64, 64, 16);
    CHECK_FALSE(grid.commit(settings).full);
    grid.begin(80, 64, 16);
    CHECK(grid.commit(settings).full);
    grid.begin(80, 64, 8);
    CHECK(grid.commit(settings).full);
}

TEST_CASE("damage over the full fraction of the tiles repaints everything") {
    DamageGrid grid;
    grid.begin(64, 64, 16);
    grid.commit(settings);
    grid.begin(64, 64, 16);
    grid.add({0.0, 0.0, 64.0, 40.0}, 9);
    const DamageGrid::Result r = grid.commit(settings);
    CHECK(r.full);
    CHECK(r.tiles == 12);
    REQUIRE(r.rects.size() == 1);
    CHECK(r.rects[0] == PixelRect{0, 0, 64, 64});
}

TEST_CASE("dirty tiles merge into rows and columns, are cut to the canvas and are capped in number") {
    const int columns = 5;
    const int rows = 4;
    std::vector<std::uint8_t> dirty(columns * rows, 0);
    for (int r = 0; r < 3; ++r) {
        dirty[r * columns + 1] = 1;
        dirty[r * columns + 2] = 1;
    }
    dirty[3 * columns + 4] = 1;
    const std::vector<PixelRect> merged = merge_tiles(dirty, columns, rows, 10, 45, 38, 4, 48);
    REQUIRE(merged.size() == 2);
    CHECK(merged[0] == PixelRect{10, 0, 20, 30});
    CHECK(merged[1] == PixelRect{40, 30, 5, 8});

    std::vector<std::uint8_t> scattered(columns * rows, 0);
    for (int i = 0; i < columns * rows; i += 2) {
        scattered[i] = 1;
    }
    const std::vector<PixelRect> capped = merge_tiles(scattered, columns, rows, 10, 50, 40, 3, 48);
    CHECK(capped.size() == 3);
    for (int i = 0; i < columns * rows; i += 2) {
        CHECK(covers(capped, (i % columns) * 10 + 5, (i / columns) * 10 + 5));
    }
    const std::vector<PixelRect> collapsed = merge_tiles(scattered, columns, rows, 10, 50, 40, 3, 4);
    REQUIRE(collapsed.size() == 1);
    CHECK(collapsed[0] == PixelRect{0, 0, 50, 40});
    CHECK(total_area(merge_tiles(std::vector<std::uint8_t>(columns * rows, 0), columns, rows, 10, 50, 40, 3, 48)) == 0);
}

TEST_CASE("rectangles can be made whole rows of tiles") {
    const int columns = 6;
    const int rows = 3;
    std::vector<std::uint8_t> dirty(columns * rows, 0);
    dirty[0 * columns + 2] = 1;
    dirty[0 * columns + 4] = 1;
    dirty[1 * columns + 4] = 1;
    dirty[2 * columns + 1] = 1;
    const std::vector<PixelRect> anchored = merge_tiles(dirty, columns, rows, 10, 60, 30, 4, 48, true);
    REQUIRE(anchored.size() == 1);
    CHECK(anchored[0] == PixelRect{0, 0, 60, 30});

    DamageGrid grid;
    DamageSettings left = settings;
    left.whole_rows = true;
    grid.begin(160, 160, 16);
    grid.commit(left);
    grid.begin(160, 160, 16);
    grid.add({100.0, 100.0, 10.0, 10.0}, 5);
    const DamageGrid::Result r = grid.commit(left);
    REQUIRE(r.rects.size() == 1);
    CHECK(r.rects[0] == PixelRect{0, 96, 160, 16});
}