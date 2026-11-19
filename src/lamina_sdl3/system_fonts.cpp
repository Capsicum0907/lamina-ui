#include "lamina_sdl3/system_fonts.hpp"

#include <stdexcept>
#include <string>

#include <SDL3/SDL.h>

namespace lamina::platform {

std::vector<std::filesystem::path> system_font_directories() {
    std::vector<std::filesystem::path> dirs;
    for (const auto& d : font_directories) {
        const char* value = SDL_getenv(std::string(d.environment).c_str());
        if (!value || !*value) {
            continue;
        }
        const std::string narrow(value);
        std::filesystem::path dir = std::filesystem::path(std::u8string(narrow.begin(), narrow.end())) / d.subdirectory;
        dir = dir.lexically_normal();
        bool seen = false;
        for (const auto& known : dirs) {
            seen = seen || known == dir;
        }
        if (!seen) {
            dirs.push_back(std::move(dir));
        }
    }
    return dirs;
}

text::FontStack system_font_stack(std::span<const SystemFont> candidates) {
    const auto dirs = system_font_directories();
    std::string tried;
    for (const auto& font : candidates) {
        for (const auto& dir : dirs) {
            const std::filesystem::path path = dir / font.file;
            std::error_code ec;
            if (std::filesystem::is_regular_file(path, ec)) {
                return text::FontStack({text::Font::from_file(path.string(), font.face_index)});
            }
            tried += "\n  " + path.string();
        }
    }
    if (dirs.empty()) {
        tried += "\n  (no font directory: none of the environment variables is set)";
    }
    throw std::runtime_error("cannot find a system font for the UI text; pass font files on the command line. Looked for:" + tried);
}

text::FontStack font_stack_from_arguments(int argc, char** argv) {
    if (argc <= 1) {
        return system_font_stack();
    }
    text::FontStack fonts;
    for (int i = 1; i < argc; ++i) {
        fonts.add(text::Font::from_file(argv[i]));
    }
    return fonts;
}

}
