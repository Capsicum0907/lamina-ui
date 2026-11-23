#include "lamina_sdl3/report.hpp"

#include <cstdio>
#include <string>

#include <SDL3/SDL.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace lamina::sdl3 {

void report_error(std::string_view message) {
    const std::string text(message);
#ifdef __ANDROID__
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", text.c_str());
#else
    std::fprintf(stderr, "%s\n", text.c_str());
#endif
}

void report_info(std::string_view tag, std::string_view message) {
    const std::string text(message);
#ifdef __ANDROID__
    __android_log_write(ANDROID_LOG_INFO, std::string(tag).c_str(), text.c_str());
#else
    std::fprintf(stderr, "%.*s: %s\n", static_cast<int>(tag.size()), tag.data(), text.c_str());
    std::fflush(stderr);
#endif
}

}
