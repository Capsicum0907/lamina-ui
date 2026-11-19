#pragma once

#include <string>
#include <string_view>

namespace lamina::platform {

class Clipboard {
public:
    virtual ~Clipboard() = default;

    virtual std::string text() = 0;
    virtual void set_text(std::string_view utf8) = 0;
};

}
