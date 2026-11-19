#include "lamina/text/line_map.hpp"

#include <utility>

namespace lamina::text {

LineMap::LineMap(std::u32string_view text, std::vector<LineSpan> spans) : spans_(std::move(spans)) {
    limits_.reserve(spans_.size());
    for (std::size_t k = 0; k < spans_.size(); ++k) {
        std::size_t end = k + 1 < spans_.size() ? spans_[k + 1].first : text.size();
        if (end > spans_[k].last && text[end - 1] == U'\n') {
            --end;
        }
        limits_.push_back(end);
    }
}

bool LineMap::wraps_at(std::size_t line, std::size_t index) const {
    return line + 1 < spans_.size() && spans_[line + 1].first == index && limits_[line] == index;
}

std::size_t LineMap::line_of(std::size_t index, bool upstream) const {
    std::size_t k = 0;
    while (k + 1 < spans_.size() && spans_[k + 1].first <= index) {
        ++k;
    }
    if (upstream && k > 0 && wraps_at(k - 1, index)) {
        --k;
    }
    return k;
}

}
