#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "lamina/color/argb.hpp"
#include "lamina/color/scheme.hpp"
#include "lamina/diagnostics/frame_stats.hpp"
#include "lamina/icons/icon.hpp"
#include "lamina/input/caption.hpp"
#include "lamina/input/clipboard.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/text/font.hpp"
#include "lamina/text/typeface.hpp"
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
    virtual void set_dark_theme(bool) {}
    virtual void system_back() {}
    virtual bool frame_stats() const { return false; }
    virtual void set_frame_stats(bool) {}
    virtual diagnostics::FrameProbe* frame_probe() { return nullptr; }
};

struct GalleryStart {
    bool dark = false;
};

struct GalleryFonts {
    std::function<text::FontStack(text::Typeface)> load;
    text::Typeface typeface = text::default_typeface;
};

class Gallery {
public:
    Gallery(const text::FontStack& fonts, const icons::IconFont& icon_font, GalleryHost& host, GalleryStart start = {});
    Gallery(GalleryFonts fonts, const icons::IconFont& icon_font, GalleryHost& host, GalleryStart start = {});
    ~Gallery();
    Gallery(const Gallery&) = delete;
    Gallery& operator=(const Gallery&) = delete;

    color::Argb update(const sdl3::Frame& frame);
    bool paint(render::Canvas& canvas, const sdl3::Frame& frame);

    std::optional<render::Rect> caret_area() const;
    const color::ColorScheme& scheme() const;
    const text::FontStack& fonts() const;
    std::optional<double> next_wake_seconds() const;
    std::optional<input::Cursor> cursor() const;
    std::vector<widgets::AccessibleNode> accessible_nodes() const;
    std::vector<widgets::AccessibleNode> page_nodes() const;
    std::vector<render::Rect> layout_blocks() const;
    render::Rect page_area() const;
    std::optional<render::Rect> find(std::string_view name) const;
    void set_dark(bool dark);
    void set_typeface(text::Typeface typeface);
    bool set_enabled(std::string_view name, bool enabled);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
