#include "lamina/style/style.hpp"

#include <atomic>

namespace lamina {

namespace {

std::atomic<std::uint64_t> next_revision{1};
std::atomic<const Style*> app_default{nullptr};
thread_local const Style* active = nullptr;

std::uint64_t take_revision() {
    return next_revision.fetch_add(1, std::memory_order_relaxed);
}

}

StyleRevision::StyleRevision() : value_(take_revision()) {}

StyleRevision::StyleRevision(const StyleRevision&) : value_(take_revision()) {}

StyleRevision& StyleRevision::operator=(const StyleRevision&) {
    value_ = take_revision();
    return *this;
}

void StyleRevision::bump() {
    value_ = take_revision();
}

const Style& m3_expressive_style() {
    static const Style style;
    return style;
}

const Style& default_style() {
    const Style* s = app_default.load(std::memory_order_acquire);
    return s ? *s : m3_expressive_style();
}

void set_default_style(const Style& style) {
    app_default.store(&style, std::memory_order_release);
}

void reset_default_style() {
    app_default.store(nullptr, std::memory_order_release);
}

const Style& current_style() {
    return active ? *active : default_style();
}

void set_current_style(const Style& style) {
    active = &style;
}

void clear_current_style() {
    active = nullptr;
}

StyleScope::StyleScope(const Style& style) : previous_(active) {
    active = &style;
}

StyleScope::~StyleScope() {
    active = previous_;
}

bool StyleStamp::refresh() {
    const std::uint64_t now = current_style().revision.value();
    if (now == seen_) {
        return false;
    }
    seen_ = now;
    return true;
}

}
