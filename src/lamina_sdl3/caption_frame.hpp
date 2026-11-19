#pragma once

#include <memory>

#include <SDL3/SDL.h>

#include "lamina/input/caption.hpp"
#include "lamina/input/events.hpp"

namespace lamina::sdl3::detail {

class CaptionFrame {
public:
    virtual ~CaptionFrame() = default;
    virtual void set_regions(input::CaptionRegions regions) = 0;
};

bool caption_frame_supported();
std::unique_ptr<CaptionFrame> attach_caption_frame(SDL_Window* window, Uint32 pointer_event);

}
