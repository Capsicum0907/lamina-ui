#include "common/options.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/tokens/title_bar.hpp"

namespace lamina::examples {

bool take_flag(int& argc, char** argv, std::string_view flag) {
    char** end = argv + argc;
    char** kept = std::remove_if(argv + std::min(argc, 1), end, [flag](const char* a) { return a == flag; });
    const bool found = kept != end;
    argc = static_cast<int>(kept - argv);
    return found;
}

int window_height_dp(int content_height_dp, bool title_bar) {
    return content_height_dp + (title_bar ? static_cast<int>(std::ceil(tokens::title_bar_tokens.height)) : 0);
}

}
