#include "common/options.hpp"

#include <algorithm>
#include <cmath>

#include "lamina/tokens/title_bar.hpp"
#include "lamina_sdl3/window.hpp"

namespace lamina::examples {

bool take_flag(int& argc, char** argv, std::string_view flag) {
    char** end = argv + argc;
    char** kept = std::remove_if(argv + std::min(argc, 1), end, [flag](const char* a) { return a == flag; });
    const bool found = kept != end;
    argc = static_cast<int>(kept - argv);
    return found;
}

std::optional<std::string> take_option(int& argc, char** argv, std::string_view name) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == name) {
            std::string value = argv[i + 1];
            std::copy(argv + i + 2, argv + argc, argv + i);
            argc -= 2;
            return value;
        }
    }
    return std::nullopt;
}

int window_height_dp(int content_height_dp, bool title_bar) {
    return content_height_dp + (title_bar && sdl3::client_title_bar_supported() ? static_cast<int>(std::ceil(tokens::title_bar_tokens.height)) : 0);
}

}
