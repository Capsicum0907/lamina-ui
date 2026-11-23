#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "lamina/color/argb.hpp"

namespace lamina::render {

struct ImageIdentity {
    std::uint64_t serial = 0;
};

struct Image {
    int width = 0;
    int height = 0;
    std::vector<color::Argb> pixels;
    std::shared_ptr<const ImageIdentity> identity = nullptr;
};

inline Image retained(Image image) {
    static std::atomic<std::uint64_t> next{1};
    image.identity = std::make_shared<const ImageIdentity>(ImageIdentity{next.fetch_add(1)});
    return image;
}

}
