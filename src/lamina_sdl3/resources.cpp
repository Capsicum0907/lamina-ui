#include "lamina_sdl3/resources.hpp"

#include <stdexcept>
#include <string>

#include <SDL3/SDL.h>

namespace lamina::sdl3 {

namespace {

std::filesystem::path from_utf8(const char* s) {
    const std::string narrow(s);
    return std::filesystem::path(std::u8string(narrow.begin(), narrow.end()));
}

}

std::filesystem::path preference_directory(const std::string& organization, const std::string& application) {
    char* raw = SDL_GetPrefPath(organization.c_str(), application.c_str());
    if (!raw) {
        throw std::runtime_error("cannot find the preference directory for " + organization + "/" + application + ": " + SDL_GetError());
    }
    std::filesystem::path dir = from_utf8(raw);
    SDL_free(raw);
    return dir;
}

}
