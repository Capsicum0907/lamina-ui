#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "lamina/color/argb.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/caption.hpp"
#include "lamina/input/clipboard.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/widgets/accessibility.hpp"
#include "lamina_sdl3/window.hpp"

namespace lamina::examples::components {

inline constexpr int window_width_dp = 1440;
inline constexpr int window_height_dp = 860;

class GalleryHost {
public:
    virtual ~GalleryHost() = default;
    virtual input::Clipboard* clipboard() = 0;
    virtual bool client_title_bar() const { return false; }
    virtual void set_caption_regions(input::CaptionRegions) {}
    virtual void minimize() {}
    virtual void toggle_maximize() {}
    virtual void close() {}
};

class Gallery {
public:
    Gallery(const text::FontStack& fonts, const icons::IconFont& icon_font, GalleryHost& host);
    ~Gallery();
    Gallery(const Gallery&) = delete;
    Gallery& operator=(const Gallery&) = delete;

    color::Argb update(const sdl3::Frame& frame);
    bool paint(render::Canvas& canvas, const sdl3::Frame& frame);

    std::optional<render::Rect> caret_area() const;
    std::optional<double> next_wake_seconds() const;
    std::vector<widgets::AccessibleNode> accessible_nodes() const;
    std::optional<render::Rect> find(std::string_view name) const;
    void set_dark(bool dark);
    bool set_enabled(std::string_view name, bool enabled);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
