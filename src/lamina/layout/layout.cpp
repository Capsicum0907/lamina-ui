#include "lamina/layout/layout.hpp"

#include <algorithm>
#include <cmath>

namespace lamina::layout {

namespace {

double finite_or(double v, double fallback) {
    return std::isfinite(v) ? v : fallback;
}

}

Size Constraints::constrain(Size s) const {
    return {std::clamp(s.width, min_width, std::max(min_width, max_width)),
            std::clamp(s.height, min_height, std::max(min_height, max_height))};
}

Size Node::measure(const Constraints& c) {
    measured_ = on_measure(c);
    return measured_;
}

void Node::arrange(const render::Rect& rect) {
    rect_ = rect;
    on_arrange(rect);
}

Size Linear::on_measure(const Constraints& c) {
    const bool horizontal = axis_ == Axis::horizontal;
    const double pad_main = horizontal ? padding_.left + padding_.right : padding_.top + padding_.bottom;
    const double pad_cross = horizontal ? padding_.top + padding_.bottom : padding_.left + padding_.right;
    const double max_main = (horizontal ? c.max_width : c.max_height) - pad_main;
    const double max_cross = (horizontal ? c.max_height : c.max_width) - pad_cross;
    const double min_cross = std::max(0.0, (horizontal ? c.min_height : c.min_width) - pad_cross);
    const bool bounded_cross = std::isfinite(max_cross);
    const bool fixed_cross = fill_cross_ && bounded_cross;

    auto stretches = [&](const NodePtr& child) { return child->self_align.value_or(cross_align_) == CrossAlign::stretch; };
    auto main_limited = [&](Constraints cc, double main_min, double main_max) {
        if (horizontal) {
            cc.min_width = main_min;
            cc.max_width = main_max;
        } else {
            cc.min_height = main_min;
            cc.max_height = main_max;
        }
        return cc;
    };
    auto cross_limits = [&](bool tight, double size) {
        return horizontal ? Constraints{0.0, unbounded, tight ? size : 0.0, size}
                          : Constraints{tight ? size : 0.0, size, 0.0, unbounded};
    };

    const double gaps = children_.empty() ? 0.0 : spacing_ * static_cast<double>(children_.size() - 1);
    double used = gaps;
    double total_weight = 0.0;
    double cross_size = fixed_cross ? max_cross : 0.0;
    std::vector<double> main_sizes(children_.size(), 0.0);

    for (std::size_t i = 0; i < children_.size(); ++i) {
        const NodePtr& child = children_[i];
        if (child->weight > 0.0) {
            total_weight += child->weight;
            continue;
        }
        const bool tight = fixed_cross && stretches(child);
        const Constraints cc = main_limited(cross_limits(tight, max_cross), 0.0, std::max(0.0, max_main - used));
        const Size s = child->measure(cc);
        main_sizes[i] = main(s);
        used += main(s);
        cross_size = std::max(cross_size, cross(s));
    }

    if (total_weight > 0.0) {
        const double remaining = std::isfinite(max_main) ? std::max(0.0, max_main - used) : 0.0;
        for (std::size_t i = 0; i < children_.size(); ++i) {
            const NodePtr& child = children_[i];
            if (child->weight <= 0.0) {
                continue;
            }
            const double share = remaining * child->weight / total_weight;
            const bool tight = fixed_cross && stretches(child);
            const Size s = child->measure(main_limited(cross_limits(tight, max_cross), share, share));
            main_sizes[i] = main(s);
            used += main(s);
            cross_size = std::max(cross_size, cross(s));
        }
    }

    cross_size = std::max(cross_size, min_cross);
    if (bounded_cross) {
        cross_size = std::min(cross_size, max_cross);
    }
    if (!fixed_cross) {
        for (std::size_t i = 0; i < children_.size(); ++i) {
            const NodePtr& child = children_[i];
            if (stretches(child) && cross(child->measured()) != cross_size) {
                child->measure(main_limited(cross_limits(true, cross_size), main_sizes[i], main_sizes[i]));
            }
        }
    }

    content_main_ = used;
    const double main_size = (fill_main_ || total_weight > 0.0) && std::isfinite(max_main) ? max_main : used;
    return c.constrain(make(main_size + pad_main, cross_size + pad_cross));
}

void Linear::on_arrange(const render::Rect& rect) {
    const bool horizontal = axis_ == Axis::horizontal;
    const double inner_x = rect.x + padding_.left;
    const double inner_y = rect.y + padding_.top;
    const double inner_w = std::max(0.0, rect.width - padding_.left - padding_.right);
    const double inner_h = std::max(0.0, rect.height - padding_.top - padding_.bottom);
    const double inner_main = horizontal ? inner_w : inner_h;
    const double inner_cross = horizontal ? inner_h : inner_w;

    const double free = std::max(0.0, inner_main - content_main_);
    const auto n = static_cast<double>(children_.size());
    double start = 0.0;
    double extra_gap = 0.0;
    switch (main_align_) {
    case MainAlign::start:
        break;
    case MainAlign::center:
        start = free / 2.0;
        break;
    case MainAlign::end:
        start = free;
        break;
    case MainAlign::space_between:
        extra_gap = n > 1 ? free / (n - 1) : 0.0;
        break;
    case MainAlign::space_around:
        extra_gap = n > 0 ? free / n : 0.0;
        start = extra_gap / 2.0;
        break;
    case MainAlign::space_evenly:
        extra_gap = free / (n + 1);
        start = extra_gap;
        break;
    }

    double pos = start;
    for (const NodePtr& child : children_) {
        const Size s = child->measured();
        const CrossAlign align = child->self_align.value_or(cross_align_);
        double cross_size = cross(s);
        double cross_pos = 0.0;
        switch (align) {
        case CrossAlign::start:
            break;
        case CrossAlign::center:
            cross_pos = (inner_cross - cross_size) / 2.0;
            break;
        case CrossAlign::end:
            cross_pos = inner_cross - cross_size;
            break;
        case CrossAlign::stretch:
            cross_size = inner_cross;
            break;
        }
        const double m = main(s);
        if (horizontal) {
            child->arrange({inner_x + pos, inner_y + cross_pos, m, cross_size});
        } else {
            child->arrange({inner_x + cross_pos, inner_y + pos, cross_size, m});
        }
        pos += m + spacing_ + extra_gap;
    }
}

Size Stack::on_measure(const Constraints& c) {
    Size size{0.0, 0.0};
    for (const NodePtr& child : children_) {
        const Size s = child->measure(c.loosen());
        size.width = std::max(size.width, s.width);
        size.height = std::max(size.height, s.height);
    }
    return c.constrain(size);
}

void Stack::on_arrange(const render::Rect& rect) {
    for (const NodePtr& child : children_) {
        child->arrange(rect);
    }
}

std::shared_ptr<Linear> row(std::vector<NodePtr> children) {
    return std::make_shared<Linear>(Axis::horizontal, std::move(children));
}

std::shared_ptr<Linear> column(std::vector<NodePtr> children) {
    return std::make_shared<Linear>(Axis::vertical, std::move(children));
}

NodePtr spacer(Size size) {
    return std::make_shared<Spacer>(size);
}

NodePtr flexible_space(double weight) {
    auto s = std::make_shared<Spacer>();
    s->weight = weight;
    return s;
}

NodePtr fixed(Size size, Leaf::ArrangeFn arrange) {
    return std::make_shared<Leaf>([size](const Constraints&) { return size; }, std::move(arrange));
}

void run(Node& root, const render::Rect& area) {
    const Size s = root.measure({0.0, area.width, 0.0, area.height});
    root.arrange({area.x, area.y, finite_or(s.width, area.width), finite_or(s.height, area.height)});
}

}
