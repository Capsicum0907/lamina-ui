#include "lamina/style/style.hpp"

namespace lamina {

namespace {

thread_local const Style* active = nullptr;

}

const Style& m3_expressive_style() {
    static const Style style;
    return style;
}

const Style& current_style() {
    return active ? *active : m3_expressive_style();
}

void set_current_style(const Style& style) {
    active = &style;
}

StyleScope::StyleScope(const Style& style) : previous_(active) {
    active = &style;
}

StyleScope::~StyleScope() {
    active = previous_;
}

}
