#pragma once

#include <SDL3/SDL.h>

namespace lamina::sdl3::detail {

void set_dark_system_bars(bool dark);
bool is_system_back(SDL_Keycode key);
void system_back();

}
