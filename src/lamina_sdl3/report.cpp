#include "lamina_sdl3/report.hpp"

#include <cstdio>
#include <string>

#include <SDL3/SDL.h>

namespace lamina::sdl3 {

void report_error(std::string_view message) {
    const std::string text(message);
#ifdef __ANDROID__
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", text.c_str());
#else
    std::fprintf(stderr, "%s\n", text.c_str());
#endif
}

}
