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

std::filesystem::path executable_directory() {
    const char* base = SDL_GetBasePath();
    if (!base) {
        throw std::runtime_error(std::string("cannot find the executable's directory: ") + SDL_GetError());
    }
    return from_utf8(base);
}

std::filesystem::path find_asset(const std::filesystem::path& relative) {
    std::string tried;
    std::filesystem::path dir = executable_directory().lexically_normal();
    if (!dir.has_filename()) {
        dir = dir.parent_path();
    }
    while (true) {
        const std::filesystem::path candidate = dir / assets_directory / relative;
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec)) {
            return candidate;
        }
        tried += "\n  " + candidate.string();
        const std::filesystem::path parent = dir.parent_path();
        if (parent.empty() || parent == dir) {
            break;
        }
        dir = parent;
    }
    throw std::runtime_error("cannot find the asset " + relative.string() + "; looked in:" + tried);
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

icons::IconFont bundled_icon_font() {
    return icons::IconFont::from_file(find_asset(std::filesystem::path(icons_directory) / icons::symbols_font_file).string());
}

}
