#pragma once

#include <optional>

#include <SDL3/SDL.h>

#include "lamina/layout/layout.hpp"

namespace lamina::sdl3::detail {

struct SystemInsets {
    layout::Insets content;
    layout::Insets gestures;
};

void set_dark_system_bars(bool dark);
bool is_system_back(SDL_Keycode key);
void system_back();
std::optional<SystemInsets> system_insets_pixels();

}
