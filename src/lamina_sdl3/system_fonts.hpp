#pragma once

#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

#include "lamina/text/font.hpp"
#include "lamina/text/typeface.hpp"

namespace lamina::sdl3 {

struct FontDirectory {
    std::string_view environment;
    std::string_view subdirectory;
};

struct SystemFont {
    std::string_view file;
    unsigned face_index;
};

inline constexpr FontDirectory windows_font_directories[] = {
    {"WINDIR", "Fonts"},
    {"SystemRoot", "Fonts"},
    {"LOCALAPPDATA", "Microsoft/Windows/Fonts"},
};

inline constexpr SystemFont windows_japanese_ui_fonts[] = {
    {"NotoSansJP-VF.ttf", 0},
    {"YuGothM.ttc", 1},
    {"meiryo.ttc", 2},
    {"BIZ-UDGothicR.ttc", 1},
    {"msgothic.ttc", 1},
};

inline constexpr FontDirectory android_font_directories[] = {
    {"ANDROID_ROOT", "fonts"},
};

inline constexpr SystemFont android_japanese_ui_fonts[] = {
    {"NotoSansCJK-Regular.ttc", 0},
};

#ifdef __ANDROID__
inline constexpr auto& font_directories = android_font_directories;
inline constexpr auto& japanese_ui_fonts = android_japanese_ui_fonts;
#else
inline constexpr auto& font_directories = windows_font_directories;
inline constexpr auto& japanese_ui_fonts = windows_japanese_ui_fonts;
#endif

std::vector<std::filesystem::path> system_font_directories();
text::FontStack system_font_stack(std::span<const SystemFont> candidates = japanese_ui_fonts);
text::FontStack system_fallback_stack(std::span<const SystemFont> candidates = japanese_ui_fonts);
text::FontStack typeface_stack(text::Typeface typeface, std::span<const text::EmbeddedFont> embedded);
text::FontStack font_stack_from_arguments(int argc, char** argv);

}
