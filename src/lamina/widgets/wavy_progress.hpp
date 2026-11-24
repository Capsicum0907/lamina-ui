#pragma once

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "lamina/color/scheme.hpp"
#include "lamina/motion/tween.hpp"
#include "lamina/render/canvas.hpp"
#include "lamina/render/polyline.hpp"
#include "lamina/style/style.hpp"
#include "lamina/tokens/wavy_progress.hpp"
#include "lamina/widgets/accessibility.hpp"

namespace lamina::widgets {

double wavy_indicator_amplitude(double progress);
double linear_indeterminate_fraction(const tokens::KeyframeSegment& segment, double play_ms);
double circular_indeterminate_sweep(double play_ms);
double circular_indeterminate_rotation(double play_ms);

struct WavyProgressOptions {
    std::optional<double> progress = std::nullopt;
    std::optional<double> wavelength = std::nullopt;
    std::optional<double> wave_speed = std::nullopt;
    std::function<double(double)> amplitude = wavy_indicator_amplitude;
    double indeterminate_amplitude = 1.0;
    color::Role indicator = current_style().wavy_progress.indicator;
    color::Role track = current_style().wavy_progress.track;
    std::string accessible_name = {};
};

class WavyProgressCore {
public:
    explicit WavyProgressCore(WavyProgressOptions options);
    virtual ~WavyProgressCore() = default;
    WavyProgressCore(const WavyProgressCore&) = default;
    WavyProgressCore& operator=(const WavyProgressCore&) = default;

    bool determinate() const { return target_.has_value(); }
    std::optional<double> target() const { return target_; }
    double progress() const;
    void set_progress(std::optional<double> progress);
    void ease_progress(double progress);
    double amplitude() const { return amplitude_.value(); }
    double wave_offset() const { return offset_; }
    double play_ms() const { return play_ms_; }
    double handoff() const { return handoff_.value(); }
    const WavyProgressOptions& options() const { return options_; }
    AccessibleNode accessible(const render::Rect& bounds) const;

    void step(double dt_seconds, double offset_period_seconds);
    bool animating() const;

protected:
    virtual void on_handoff() {}
    double target_amplitude() const;
    WavyProgressOptions options_;
    std::optional<double> target_;
    motion::Tweened shown_;
    motion::Tweened amplitude_;
    motion::Tweened handoff_;
    double offset_ = 0.0;
    double play_ms_ = 0.0;
    bool eased_ = false;
};

class LinearWavyProgressIndicator : public WavyProgressCore {
public:
    explicit LinearWavyProgressIndicator(WavyProgressOptions options = {});

    double width() const { return width_; }
    double height() const { return current_style().wavy_progress.linear_height; }
    void set_width(double width) { width_ = width; }
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const { return {origin_.x, origin_.y, width_, height()}; }
    double wavelength() const;
    std::array<double, 4> fractions() const;
    std::vector<std::vector<render::Point>> active_lines() const;
    std::vector<std::vector<render::Point>> track_lines() const;
    std::optional<render::Rect> stop_indicator() const;

    AccessibleNode accessible() const { return WavyProgressCore::accessible(bounds()); }
    void step(double dt_seconds);
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    void on_handoff() override;
    double cap() const;
    const render::Polyline& wave() const;

    double width_ = current_style().wavy_progress.linear_width;
    render::Point origin_{0.0, 0.0};
    mutable render::Polyline wave_;
    mutable double wave_width_ = -1.0;
    mutable double wave_length_ = -1.0;
    mutable bool wave_flat_ = true;
    mutable double wave_scale_ = 1.0;
};

class CircularWavyProgressIndicator : public WavyProgressCore {
public:
    explicit CircularWavyProgressIndicator(WavyProgressOptions options = {});

    double width() const { return size_; }
    double height() const { return size_; }
    void set_size(double size) { size_ = size; }
    void set_origin(render::Point origin) { origin_ = origin; }
    render::Rect bounds() const { return {origin_.x, origin_.y, size_, size_}; }
    double wavelength() const;
    int vertex_count() const;
    double sweep() const;
    double rotation_degrees() const;
    std::vector<render::Point> active_line() const;
    std::vector<render::Point> track_line() const;

    AccessibleNode accessible() const { return WavyProgressCore::accessible(bounds()); }
    void step(double dt_seconds);
    void paint(render::Canvas& canvas, const color::ColorScheme& scheme) const;

private:
    void on_handoff() override;
    std::vector<render::Point> outline(bool active, bool repeat) const;
    render::Point centre() const { return {origin_.x + size_ / 2.0, origin_.y + size_ / 2.0}; }

    double size_ = current_style().wavy_progress.circular_size;
    render::Point origin_{0.0, 0.0};
    double handoff_sweep_ = 0.0;
    double handoff_rotation_ = 0.0;
};

}
