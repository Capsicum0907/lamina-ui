#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "common/animated_box.hpp"
#include "lamina/layout/layout.hpp"
#include "lamina/layout/window_size_class.hpp"
#include "lamina/tokens/motion.hpp"

namespace lamina::examples::components {

struct ReflowStyle {
    motion::SpringSpec spring = tokens::motion_scheme.default_spatial();
    layout::WindowWidthClass columns_from = layout::WindowWidthClass::expanded;
    layout::WindowWidthClass caption_beside_from = layout::WindowWidthClass::medium;
};

inline const ReflowStyle reflow_style;

class Reflow;

class Placement final : public layout::Node {
public:
    Placement(Reflow& reflow, layout::NodePtr child, motion::SpringSpec spring) : reflow_(reflow), child_(std::move(child)), box_(spring) {}

    void step(double dt) { box_.step(dt); }
    bool settled() const { return box_.settled(); }
    bool holds_placements() const { return holds_; }

protected:
    layout::Size on_measure(const layout::Constraints& c) override { return child_->measure(c); }
    void on_arrange(const render::Rect& r) override;

private:
    Reflow& reflow_;
    layout::NodePtr child_;
    AnimatedBox box_;
    bool holds_ = false;
};

class Reflow {
public:
    explicit Reflow(const layout::WindowSizeClass& size_class, const ReflowStyle& style = reflow_style) : size_class_(size_class), style_(style) {}
    Reflow(const Reflow&) = delete;
    Reflow& operator=(const Reflow&) = delete;

    const layout::WindowSizeClass& size_class() const { return size_class_; }
    const ReflowStyle& style() const { return style_; }
    bool columns() const { return size_class_.is_width_at_least(style_.columns_from); }
    bool caption_beside() const { return size_class_.is_width_at_least(style_.caption_beside_from); }

    layout::NodePtr place(layout::NodePtr child) {
        placements_.push_back(std::make_shared<Placement>(*this, std::move(child), style_.spring));
        return placements_.back();
    }

    std::shared_ptr<layout::Flow> flow(std::vector<layout::NodePtr> children) {
        for (layout::NodePtr& c : children) {
            c = place(std::move(c));
        }
        flows_.push_back(layout::flow(std::move(children)));
        return flows_.back();
    }

    layout::NodePtr root(layout::NodePtr child);

    void snap() { snap_ = true; }
    bool snapping() const { return snap_; }
    bool switching() const { return switching_; }

    void step(double dt) {
        for (const auto& p : placements_) {
            p->step(dt);
        }
    }
    bool animating() const {
        return std::any_of(placements_.begin(), placements_.end(), [](const auto& p) { return !p->settled(); });
    }

    render::Point anchor() const { return anchors_.empty() ? render::Point{0.0, 0.0} : anchors_.back(); }
    void push_anchor(render::Point p) { anchors_.push_back(p); }
    void pop_anchor() { anchors_.pop_back(); }
    Placement* enter(Placement* p) {
        Placement* outer = open_.empty() ? nullptr : open_.back();
        open_.push_back(p);
        return outer;
    }
    void leave() { open_.pop_back(); }

    std::vector<render::Rect> blocks() const {
        std::vector<render::Rect> out;
        for (const auto& p : placements_) {
            if (!p->holds_placements() && p->rect().width > 0.0 && p->rect().height > 0.0) {
                out.push_back(p->rect());
            }
        }
        return out;
    }

    std::vector<double> signature() const {
        std::vector<double> s{static_cast<double>(size_class_.width_class())};
        for (const auto& f : flows_) {
            s.push_back(static_cast<double>(f->line_count()));
            for (std::size_t i = 0; i < f->line_count(); ++i) {
                s.push_back(static_cast<double>(f->line_start(i)));
            }
        }
        for (const auto& p : placements_) {
            s.push_back(p->measured().height);
        }
        return s;
    }

    void begin_arrange(std::vector<double> signature) {
        switching_ = !snap_ && signature != committed_;
        committed_ = std::move(signature);
    }
    void end_arrange() {
        snap_ = false;
        switching_ = false;
    }

private:
    const layout::WindowSizeClass& size_class_;
    const ReflowStyle& style_;
    std::vector<std::shared_ptr<Placement>> placements_;
    std::vector<std::shared_ptr<layout::Flow>> flows_;
    std::vector<render::Point> anchors_;
    std::vector<Placement*> open_;
    std::vector<double> committed_;
    bool snap_ = true;
    bool switching_ = false;
};

inline void Placement::on_arrange(const render::Rect& r) {
    const render::Point origin = reflow_.anchor();
    const render::Rect relative{r.x - origin.x, r.y - origin.y, r.width, r.height};
    if (reflow_.snapping()) {
        box_.snap_to(relative);
    } else {
        box_.move_to(relative, reflow_.switching());
    }
    const render::Rect b = box_.rect();
    const render::Rect at{origin.x + b.x, origin.y + b.y, b.width, b.height};
    holds_ = false;
    if (Placement* outer = reflow_.enter(this)) {
        outer->holds_ = true;
    }
    reflow_.push_anchor({at.x, at.y});
    child_->arrange(at);
    reflow_.pop_anchor();
    reflow_.leave();
}

class ReflowRoot final : public layout::Node {
public:
    ReflowRoot(Reflow& reflow, layout::NodePtr child) : reflow_(reflow), child_(std::move(child)) {}

protected:
    layout::Size on_measure(const layout::Constraints& c) override {
        const layout::Size s = child_->measure(c);
        pending_ = reflow_.signature();
        return s;
    }
    void on_arrange(const render::Rect& r) override {
        reflow_.begin_arrange(pending_);
        reflow_.push_anchor({r.x, r.y});
        child_->arrange(r);
        reflow_.pop_anchor();
        reflow_.end_arrange();
    }

private:
    Reflow& reflow_;
    layout::NodePtr child_;
    std::vector<double> pending_;
};

inline layout::NodePtr Reflow::root(layout::NodePtr child) {
    return std::make_shared<ReflowRoot>(*this, std::move(child));
}

class Columns final : public layout::Node {
public:
    Columns(Reflow& reflow, const std::vector<std::vector<layout::NodePtr>>& columns, double column_gap, double row_gap)
        : reflow_(reflow), column_gap_(column_gap), row_gap_(row_gap) {
        for (const auto& column : columns) {
            std::vector<layout::NodePtr> placed;
            for (const layout::NodePtr& section : column) {
                placed.push_back(reflow.place(section));
            }
            columns_.push_back(std::move(placed));
        }
    }

protected:
    layout::Size on_measure(const layout::Constraints& c) override {
        offsets_.clear();
        if (!reflow_.columns()) {
            double y = 0.0;
            double width = 0.0;
            for (const auto& column : columns_) {
                for (const layout::NodePtr& section : column) {
                    const layout::Size s = section->measure({0.0, c.max_width, 0.0, layout::unbounded});
                    y += offsets_.empty() ? 0.0 : row_gap_;
                    offsets_.push_back({0.0, y});
                    y += s.height;
                    width = std::max(width, s.width);
                }
            }
            return c.constrain({width, y});
        }
        std::vector<double> widths;
        double natural = column_gap_ * static_cast<double>(columns_.empty() ? 0 : columns_.size() - 1);
        const double gaps = natural;
        for (const auto& column : columns_) {
            double w = 0.0;
            for (const layout::NodePtr& section : column) {
                w = std::max(w, section->measure({0.0, layout::unbounded, 0.0, layout::unbounded}).width);
            }
            widths.push_back(w);
            natural += w;
        }
        if (std::isfinite(c.max_width) && natural > c.max_width) {
            const double cap = shared_cap(widths, std::max(0.0, c.max_width - gaps));
            for (double& w : widths) {
                w = std::min(w, cap);
            }
        }
        double x = 0.0;
        double height = 0.0;
        for (std::size_t i = 0; i < columns_.size(); ++i) {
            double y = 0.0;
            for (std::size_t k = 0; k < columns_[i].size(); ++k) {
                const layout::Size s = columns_[i][k]->measure({0.0, widths[i], 0.0, layout::unbounded});
                y += k == 0 ? 0.0 : row_gap_;
                offsets_.push_back({x, y});
                y += s.height;
            }
            height = std::max(height, y);
            x += widths[i] + column_gap_;
        }
        return c.constrain({x - (columns_.empty() ? 0.0 : column_gap_), height});
    }

    void on_arrange(const render::Rect& r) override {
        std::size_t n = 0;
        for (const auto& column : columns_) {
            for (const layout::NodePtr& section : column) {
                const layout::Size s = section->measured();
                section->arrange({r.x + offsets_[n].x, r.y + offsets_[n].y, s.width, s.height});
                ++n;
            }
        }
    }

private:
    static double shared_cap(std::vector<double> widths, double room) {
        std::sort(widths.begin(), widths.end());
        for (std::size_t i = 0; i < widths.size(); ++i) {
            const double count = static_cast<double>(widths.size() - i);
            if (widths[i] * count >= room) {
                return room / count;
            }
            room -= widths[i];
        }
        return widths.empty() ? 0.0 : widths.back();
    }

    Reflow& reflow_;
    std::vector<std::vector<layout::NodePtr>> columns_;
    double column_gap_;
    double row_gap_;
    std::vector<render::Point> offsets_;
};

class Captioned final : public layout::Node {
public:
    Captioned(Reflow& reflow, layout::NodePtr caption, layout::NodePtr content, double gap)
        : reflow_(reflow), caption_(reflow.place(std::move(caption))), content_(reflow.place(std::move(content))), gap_(gap) {}

protected:
    layout::Size on_measure(const layout::Constraints& c) override {
        const layout::Size cap = caption_->measure({0.0, c.max_width, 0.0, layout::unbounded});
        if (reflow_.caption_beside()) {
            const double room = std::isfinite(c.max_width) ? std::max(0.0, c.max_width - cap.width) : layout::unbounded;
            const layout::Size s = content_->measure({0.0, room, 0.0, layout::unbounded});
            const double h = std::max(cap.height, s.height);
            caption_at_ = {0.0, (h - cap.height) / 2.0};
            content_at_ = {cap.width, (h - s.height) / 2.0};
            return c.constrain({cap.width + s.width, h});
        }
        const layout::Size s = content_->measure({0.0, c.max_width, 0.0, layout::unbounded});
        caption_at_ = {0.0, 0.0};
        content_at_ = {0.0, cap.height + gap_};
        return c.constrain({std::max(cap.width, s.width), content_at_.y + s.height});
    }

    void on_arrange(const render::Rect& r) override {
        const layout::Size cap = caption_->measured();
        const layout::Size s = content_->measured();
        caption_->arrange({r.x + caption_at_.x, r.y + caption_at_.y, cap.width, cap.height});
        content_->arrange({r.x + content_at_.x, r.y + content_at_.y, s.width, s.height});
    }

private:
    Reflow& reflow_;
    layout::NodePtr caption_;
    layout::NodePtr content_;
    double gap_;
    render::Point caption_at_{0.0, 0.0};
    render::Point content_at_{0.0, 0.0};
};

inline layout::NodePtr at_most(layout::NodePtr node, double width) {
    return std::make_shared<layout::Leaf>(
        [node, width](const layout::Constraints& c) {
            const double w = std::min(width, c.max_width);
            const layout::Size s = node->measure({0.0, w, 0.0, c.max_height});
            return layout::Size{w, s.height};
        },
        [node](const render::Rect& r) { node->arrange(r); });
}

}
