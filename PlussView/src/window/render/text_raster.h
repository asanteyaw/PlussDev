#pragma once

#include <vector>

#include "pluss/view/window/renderer.h"

namespace pluss::view::window::detail {

bool render_text_command_native(PixelCanvas& canvas,
                                const TextCommand& command,
                                std::vector<RenderEvent>& events);

void render_text_command(PixelCanvas& canvas,
                         const TextCommand& command,
                         std::vector<RenderEvent>& events);

} // namespace pluss::view::window::detail
