#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/animated.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/shapes/morph.hpp"
#include "lamina/tokens/loading_indicator.hpp"
#include "lamina/widgets/accessibility.hpp"

namespace lamina::widgets {

struct LoadingIndicatorOptions {
    std::optional<double> progress = std::nullopt;
    bool contained = false;
    std::string accessible_name = {};
    double size = tokens::loading_indicator_tokens.container_size;
};

class LoadingIndicator {
public:
    explicit LoadingIndicator(LoadingIndicatorOptions options = {});

    double width() const { return options_.size; }
    double height() const { return options_.size; }
    void set_size(double size) { options_.size = size; }
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const { return {origin_.x, origin_.y, width(), height()}; }

    bool determinate() const { return options_.progress.has_value(); }
    std::optional<double> progress() const { return options_.progress; }
    void set_progress(std::optional<double> progress);
    std::size_t morph_index() const { return morph_index_; }
    double morph_progress() const;
    double rotation_degrees() const;
    std::size_t morph_count() const;
    render::Path indicator_path() const;

    void set_accessible_name(std::string name) { options_.accessible_name = std::move(name); }
    AccessibleNode accessible() const;

    void step(double dt_seconds);
    bool animating() const { return !determinate(); }
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    void start_morph();

    LoadingIndicatorOptions options_;
    render::Point origin_{0.0, 0.0};
    motion::Animated morph_;
    double morph_elapsed_ = 0.0;
    double morph_seconds_ = 0.0;
    bool morph_running_ = false;
    double cycle_elapsed_ = 0.0;
    std::size_t morph_index_ = 0;
    double turn_ = 0.0;
    double spin_ = 0.0;
};

const std::vector<shapes::Morph>& loading_indicator_morphs(bool determinate);
double loading_indicator_scale(bool determinate);
double spring_finish_seconds(const motion::SpringSpec& spec, double threshold);

}
