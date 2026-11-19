#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace lamina::text {

struct LineSpan {
    std::size_t first;
    std::size_t last;
};

class LineMap {
public:
    LineMap() = default;
    LineMap(std::u32string_view text, std::vector<LineSpan> spans);

    std::size_t size() const { return spans_.size(); }
    std::size_t first(std::size_t line) const { return spans_[line].first; }
    std::size_t limit(std::size_t line) const { return limits_[line]; }
    bool wraps_at(std::size_t line, std::size_t index) const;
    std::size_t line_of(std::size_t index, bool upstream) const;

private:
    std::vector<LineSpan> spans_;
    std::vector<std::size_t> limits_;
};

class CaretGeometry {
public:
    virtual double x_at(std::size_t line, std::size_t index) const = 0;
    virtual std::size_t index_at(std::size_t line, double x) const = 0;

protected:
    ~CaretGeometry() = default;
};

}
