#include "lamina_sdl3/system_ui.hpp"

namespace lamina::sdl3::detail {

void set_dark_system_bars(bool) {}

bool is_system_back(SDL_Keycode) {
    return false;
}

void system_back() {}

}
