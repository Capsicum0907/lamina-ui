#include <doctest/doctest.h>

#include <string_view>

#include "lamina_sdl3/system_fonts.hpp"

using namespace lamina;

TEST_CASE("the Android font table looks under ANDROID_ROOT/fonts for the CJK collection") {
    REQUIRE(std::size(sdl3::android_font_directories) == 1);
    CHECK(sdl3::android_font_directories[0].environment == std::string_view("ANDROID_ROOT"));
    CHECK(sdl3::android_font_directories[0].subdirectory == std::string_view("fonts"));
    REQUIRE(std::size(sdl3::android_japanese_ui_fonts) >= 1);
    CHECK(sdl3::android_japanese_ui_fonts[0].file == std::string_view("NotoSansCJK-Regular.ttc"));
    CHECK(sdl3::android_japanese_ui_fonts[0].face_index == 0);
}

TEST_CASE("the platform font table is the Windows one off Android") {
#ifdef __ANDROID__
    CHECK(&sdl3::font_directories == &sdl3::android_font_directories);
    CHECK(&sdl3::japanese_ui_fonts == &sdl3::android_japanese_ui_fonts);
#else
    CHECK(&sdl3::font_directories == &sdl3::windows_font_directories);
    CHECK(&sdl3::japanese_ui_fonts == &sdl3::windows_japanese_ui_fonts);
    CHECK(sdl3::windows_japanese_ui_fonts[0].file == std::string_view("NotoSansJP-VF.ttf"));
#endif
}
