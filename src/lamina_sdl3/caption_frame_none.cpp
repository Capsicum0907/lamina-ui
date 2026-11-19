#include "lamina_sdl3/caption_frame.hpp"

namespace lamina::sdl3::detail {

bool caption_frame_supported() {
    return false;
}

std::unique_ptr<CaptionFrame> attach_caption_frame(SDL_Window*, Uint32) {
    return nullptr;
}

}
