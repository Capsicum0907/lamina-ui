#pragma once

#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

#include "lamina/render/geometry.hpp"

namespace lamina::layout {

inline constexpr double unbounded = std::numeric_limits<double>::infinity();

struct Size {
    double width;
    double height;
};

struct Constraints {
    double min_width = 0.0;
    double max_width = unbounded;
    double min_height = 0.0;
    double max_height = unbounded;

    Size constrain(Size s) const;
    Constraints loosen() const { return {0.0, max_width, 0.0, max_height}; }
    static Constraints tight(Size s) { return {s.width, s.width, s.height, s.height}; }
};

struct Insets {
    double left = 0.0;
    double top = 0.0;
    double right = 0.0;
    double bottom = 0.0;

    static constexpr Insets all(double v) { return {v, v, v, v}; }
    static constexpr Insets symmetric(double horizontal, double vertical) {
        return {horizontal, vertical, horizontal, vertical};
    }
};

enum class MainAlign { start, center, end, space_between, space_around, space_evenly };
enum class CrossAlign { start, center, end, stretch };

class Node {
public:
    virtual ~Node() = default;

    Size measure(const Constraints& c);
    void arrange(const render::Rect& rect);

    Size measured() const { return measured_; }
    const render::Rect& rect() const { return rect_; }

    double weight = 0.0;
    std::optional<CrossAlign> self_align;

protected:
    virtual Size on_measure(const Constraints& c) = 0;
    virtual void on_arrange(const render::Rect& rect) = 0;

private:
    Size measured_{0.0, 0.0};
    render::Rect rect_{0.0, 0.0, 0.0, 0.0};
};

using NodePtr = std::shared_ptr<Node>;

class Leaf final : public Node {
public:
    using MeasureFn = std::function<Size(const Constraints&)>;
    using ArrangeFn = std::function<void(const render::Rect&)>;

    Leaf(MeasureFn measure, ArrangeFn arrange) : measure_(std::move(measure)), arrange_(std::move(arrange)) {}

protected:
    Size on_measure(const Constraints& c) override { return c.constrain(measure_(c)); }
    void on_arrange(const render::Rect& rect) override { arrange_(rect); }

private:
    MeasureFn measure_;
    ArrangeFn arrange_;
};

class Spacer final : public Node {
public:
    explicit Spacer(Size size = {0.0, 0.0}) : size_(size) {}

protected:
    Size on_measure(const Constraints& c) override { return c.constrain(size_); }
    void on_arrange(const render::Rect&) override {}

private:
    Size size_;
};

enum class Axis { horizontal, vertical };

class Linear : public Node {
public:
    Linear(Axis axis, std::vector<NodePtr> children) : axis_(axis), children_(std::move(children)) {}

    Linear& spacing(double v) {
        spacing_ = v;
        return *this;
    }
    Linear& padding(Insets v) {
        padding_ = v;
        return *this;
    }
    Linear& main_align(MainAlign v) {
        main_align_ = v;
        return *this;
    }
    Linear& cross_align(CrossAlign v) {
        cross_align_ = v;
        return *this;
    }
    Linear& fill_main(bool v) {
        fill_main_ = v;
        return *this;
    }
    Linear& fill_cross(bool v) {
        fill_cross_ = v;
        return *this;
    }

    const std::vector<NodePtr>& children() const { return children_; }

protected:
    Size on_measure(const Constraints& c) override;
    void on_arrange(const render::Rect& rect) override;

private:
    double main(Size s) const { return axis_ == Axis::horizontal ? s.width : s.height; }
    double cross(Size s) const { return axis_ == Axis::horizontal ? s.height : s.width; }
    Size make(double main_size, double cross_size) const {
        return axis_ == Axis::horizontal ? Size{main_size, cross_size} : Size{cross_size, main_size};
    }

    Axis axis_;
    std::vector<NodePtr> children_;
    double spacing_ = 0.0;
    Insets padding_{};
    MainAlign main_align_ = MainAlign::start;
    CrossAlign cross_align_ = CrossAlign::start;
    bool fill_main_ = false;
    bool fill_cross_ = false;
    double content_main_ = 0.0;
};

class Stack final : public Node {
public:
    explicit Stack(std::vector<NodePtr> children) : children_(std::move(children)) {}

protected:
    Size on_measure(const Constraints& c) override;
    void on_arrange(const render::Rect& rect) override;

private:
    std::vector<NodePtr> children_;
};

enum class FlowDirection { start_to_end, end_to_start };

class Flow final : public Node {
public:
    explicit Flow(std::vector<NodePtr> children) : children_(std::move(children)) {}

    Flow& spacing(double v) {
        spacing_ = v;
        return *this;
    }
    Flow& line_spacing(double v) {
        line_spacing_ = v;
        return *this;
    }
    Flow& direction(FlowDirection v) {
        direction_ = v;
        return *this;
    }

    std::size_t line_count() const { return lines_.size(); }

protected:
    Size on_measure(const Constraints& c) override;
    void on_arrange(const render::Rect& rect) override;

private:
    struct Line {
        std::size_t begin;
        std::size_t end;
        double width;
        double height;
    };

    std::vector<NodePtr> children_;
    double spacing_ = 0.0;
    double line_spacing_ = 0.0;
    FlowDirection direction_ = FlowDirection::start_to_end;
    std::vector<Line> lines_;
};

std::shared_ptr<Linear> row(std::vector<NodePtr> children);
std::shared_ptr<Linear> column(std::vector<NodePtr> children);
std::shared_ptr<Flow> flow(std::vector<NodePtr> children);
NodePtr spacer(Size size = {0.0, 0.0});
NodePtr flexible_space(double weight = 1.0);
NodePtr fixed(Size size, Leaf::ArrangeFn arrange);

template <typename T>
NodePtr weighted(T node, double w) {
    node->weight = w;
    return node;
}

void run(Node& root, const render::Rect& area);

}
