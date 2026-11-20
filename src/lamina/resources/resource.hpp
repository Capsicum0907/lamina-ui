#pragma once

#include <cstddef>
#include <span>
#include <string_view>

namespace lamina::resources {

struct Resource {
    std::string_view name;
    std::span<const unsigned char> data;

    std::span<const std::byte> bytes() const { return std::as_bytes(data); }
};

}
