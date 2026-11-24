#include "lamina_sdl3/system_ui.hpp"

namespace lamina::sdl3::detail {

void set_dark_system_bars(bool) {}

void set_system_bars_hidden(bool) {}

bool is_system_back(SDL_Keycode) {
    return false;
}

void system_back() {}

std::optional<SystemInsets> system_insets_pixels() {
    return std::nullopt;
}

void watch_new_intents() {}

bool take_new_intent() {
    return false;
}

}
