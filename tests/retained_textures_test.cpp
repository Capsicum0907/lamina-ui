#include <doctest/doctest.h>

#include <optional>
#include <set>
#include <vector>

#include "lamina/render/retained_textures.hpp"

using lamina::render::Image;
using lamina::render::RetainedTextures;
using lamina::render::retained;

namespace {

struct Fake {
    int next = 1;
    std::set<int> live;
    std::vector<int> made;
    bool refuse = false;
};

RetainedTextures<int> cache_of(Fake& fake, std::size_t budget) {
    return RetainedTextures<int>(
        budget,
        [&fake](const Image& image) -> std::optional<RetainedTextures<int>::Made> {
            if (fake.refuse) {
                return std::nullopt;
            }
            const int t = fake.next++;
            fake.live.insert(t);
            fake.made.push_back(t);
            return RetainedTextures<int>::Made{t, image.pixels.size() * 4};
        },
        [&fake](int t) { fake.live.erase(t); });
}

Image picture(int w, int h) {
    return retained(Image{w, h, std::vector<lamina::color::Argb>(static_cast<std::size_t>(w * h), 0xff000000u), nullptr});
}

}

TEST_CASE("a retained image is uploaded once and then found") {
    Fake fake;
    auto cache = cache_of(fake, 1 << 20);
    const Image a = picture(4, 4);
    const auto first = cache.get(a, 1);
    CHECK(first.made);
    REQUIRE(first.texture.has_value());
    const auto second = cache.get(a, 2);
    CHECK_FALSE(second.made);
    CHECK(second.texture == first.texture);
    const Image copy = a;
    CHECK(cache.get(copy, 3).texture == first.texture);
    CHECK(fake.made.size() == 1);
    CHECK(cache.size() == 1);
    CHECK(cache.bytes() == 64);
}

TEST_CASE("an image without an identity gets no texture") {
    Fake fake;
    auto cache = cache_of(fake, 1 << 20);
    const Image plain{2, 2, std::vector<lamina::color::Argb>(4, 0), nullptr};
    CHECK_FALSE(cache.get(plain, 1).texture.has_value());
    CHECK(fake.made.empty());
}

TEST_CASE("a texture goes when its image is gone") {
    Fake fake;
    auto cache = cache_of(fake, 1 << 20);
    std::optional<Image> a = picture(4, 4);
    cache.get(*a, 1);
    CHECK(fake.live.size() == 1);
    a.reset();
    cache.collect(1);
    CHECK(fake.live.empty());
    CHECK(cache.size() == 0);
    CHECK(cache.bytes() == 0);
}

TEST_CASE("over the budget the least recently drawn textures go, but not those drawn in this frame") {
    Fake fake;
    auto cache = cache_of(fake, 100);
    const Image a = picture(4, 4);
    const Image b = picture(4, 4);
    const Image c = picture(4, 4);
    cache.get(a, 1);
    cache.get(b, 2);
    cache.get(c, 3);
    cache.collect(3);
    CHECK(cache.bytes() <= 100);
    CHECK(cache.size() == 1);
    CHECK_FALSE(cache.get(c, 4).made);
    CHECK(cache.get(a, 4).made);
    cache.collect(4);
    CHECK(cache.size() == 2);
    CHECK(cache.bytes() == 128);
}

TEST_CASE("a refused image is remembered until it is gone") {
    Fake fake;
    auto cache = cache_of(fake, 1 << 20);
    fake.refuse = true;
    std::optional<Image> a = picture(4, 4);
    CHECK_FALSE(cache.get(*a, 1).texture.has_value());
    CHECK(cache.refused(*a));
    fake.refuse = false;
    CHECK_FALSE(cache.get(*a, 2).texture.has_value());
    a.reset();
    cache.collect(2);
    const Image b = picture(4, 4);
    CHECK_FALSE(cache.refused(b));
    CHECK(cache.get(b, 3).made);
}

TEST_CASE("clear drops every texture") {
    Fake fake;
    auto cache = cache_of(fake, 1 << 20);
    const Image a = picture(4, 4);
    const Image b = picture(2, 2);
    cache.get(a, 1);
    cache.get(b, 1);
    cache.clear();
    CHECK(fake.live.empty());
    CHECK(cache.get(a, 2).made);
}
