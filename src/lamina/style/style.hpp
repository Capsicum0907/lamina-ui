#pragma once

#include "lamina/color/scheme.hpp"

namespace lamina {

struct Style {
    const color::ColorSpec* color = &color::spec_2025();
};

const Style& m3_expressive_style();

const Style& current_style();
void set_current_style(const Style& style);

class StyleScope {
public:
    explicit StyleScope(const Style& style);
    ~StyleScope();

    StyleScope(const StyleScope&) = delete;
    StyleScope& operator=(const StyleScope&) = delete;

private:
    const Style* previous_;
};

}
