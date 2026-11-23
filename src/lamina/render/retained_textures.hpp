#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

#include "lamina/render/image.hpp"

namespace lamina::render {

template <class Texture>
class RetainedTextures {
public:
    struct Made {
        Texture texture;
        std::size_t bytes;
    };
    using Make = std::function<std::optional<Made>(const Image&)>;
    using Drop = std::function<void(Texture)>;

    struct Lookup {
        std::optional<Texture> texture;
        bool made = false;
        std::size_t bytes = 0;
    };

    RetainedTextures(std::size_t budget_bytes, Make make, Drop drop) : budget_(budget_bytes), make_(std::move(make)), drop_(std::move(drop)) {}
    ~RetainedTextures() { clear(); }
    RetainedTextures(const RetainedTextures&) = delete;
    RetainedTextures& operator=(const RetainedTextures&) = delete;

    Lookup get(const Image& image, std::uint64_t frame) {
        if (!image.identity) {
            return {};
        }
        const ImageIdentity* key = image.identity.get();
        if (const auto it = entries_.find(key); it != entries_.end()) {
            if (!it->second.owner.expired()) {
                it->second.used = frame;
                return {it->second.texture, false, it->second.bytes};
            }
            forget(it);
        }
        std::optional<Made> made = make_(image);
        Entry entry{image.identity, std::nullopt, 0, frame};
        if (made) {
            entry.texture = made->texture;
            entry.bytes = made->bytes;
            bytes_ += made->bytes;
        }
        entries_.emplace(key, std::move(entry));
        return {made ? std::optional<Texture>(made->texture) : std::nullopt, made.has_value(), made ? made->bytes : 0};
    }

    bool refused(const Image& image) const {
        if (!image.identity) {
            return false;
        }
        const auto it = entries_.find(image.identity.get());
        return it != entries_.end() && !it->second.owner.expired() && !it->second.texture;
    }

    void collect(std::uint64_t frame) {
        for (auto it = entries_.begin(); it != entries_.end();) {
            if (it->second.owner.expired()) {
                it = forget(it);
            } else {
                ++it;
            }
        }
        while (bytes_ > budget_) {
            auto oldest = entries_.end();
            for (auto it = entries_.begin(); it != entries_.end(); ++it) {
                if (it->second.texture && it->second.used != frame && (oldest == entries_.end() || it->second.used < oldest->second.used)) {
                    oldest = it;
                }
            }
            if (oldest == entries_.end()) {
                break;
            }
            forget(oldest);
        }
    }

    void clear() {
        for (auto it = entries_.begin(); it != entries_.end();) {
            it = forget(it);
        }
    }

    void set_budget(std::size_t bytes) { budget_ = bytes; }
    std::size_t budget() const { return budget_; }
    std::size_t bytes() const { return bytes_; }
    std::size_t size() const {
        std::size_t n = 0;
        for (const auto& [key, e] : entries_) {
            n += e.texture ? 1 : 0;
        }
        return n;
    }

private:
    struct Entry {
        std::weak_ptr<const ImageIdentity> owner;
        std::optional<Texture> texture;
        std::size_t bytes;
        std::uint64_t used;
    };
    using Map = std::unordered_map<const ImageIdentity*, Entry>;

    typename Map::iterator forget(typename Map::iterator it) {
        if (it->second.texture) {
            drop_(*it->second.texture);
            bytes_ -= it->second.bytes;
        }
        return entries_.erase(it);
    }

    std::size_t budget_;
    Make make_;
    Drop drop_;
    std::size_t bytes_ = 0;
    Map entries_;
};

}
