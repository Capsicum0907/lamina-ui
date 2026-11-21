#include "lamina_sdl3/system_fonts.hpp"

#include <optional>
#include <stdexcept>
#include <string>

#include <SDL3/SDL.h>

namespace lamina::sdl3 {

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

namespace {

std::optional<std::filesystem::path> find_system_font(std::span<const SystemFont> candidates, unsigned& face_index, std::string& tried) {
    const auto dirs = system_font_directories();
    for (const auto& font : candidates) {
        for (const auto& dir : dirs) {
            const std::filesystem::path path = dir / font.file;
            std::error_code ec;
            if (std::filesystem::is_regular_file(path, ec)) {
                face_index = font.face_index;
                return path;
            }
            tried += "\n  " + path.string();
        }
    }
    if (dirs.empty()) {
        tried += "\n  (no font directory: none of the environment variables is set)";
    }
    return std::nullopt;
}

}

text::FontStack system_font_stack(std::span<const SystemFont> candidates) {
    unsigned face_index = 0;
    std::string tried;
    if (const auto path = find_system_font(candidates, face_index, tried)) {
        return text::FontStack({text::Font::from_file(path->string(), face_index)});
    }
    throw std::runtime_error("cannot find a system font for the UI text; pass font files on the command line. Looked for:" + tried);
}

text::FontStack system_fallback_stack(std::span<const SystemFont> candidates) {
    unsigned face_index = 0;
    std::string tried;
    if (const auto path = find_system_font(candidates, face_index, tried)) {
        return text::FontStack({text::Font::from_file(path->string(), face_index)});
    }
    return {};
}

text::FontStack typeface_stack(text::Typeface typeface, std::span<const text::EmbeddedFont> embedded) {
    if (text::typeface_spec(typeface).chain.empty()) {
        return text::typeface_stack(typeface, embedded, system_font_stack());
    }
    return text::typeface_stack(typeface, embedded, system_fallback_stack());
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
