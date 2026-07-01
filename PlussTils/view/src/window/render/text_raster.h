#pragma once

#include <optional>
#include <vector>

#include "pluss/tils/view/window/renderer.h"

namespace pluss::view::window::detail {

[[nodiscard]] std::optional<int> measure_text_width_native(
    const TextCommand& command);

bool render_text_command_native(PixelCanvas& canvas,
                                const TextCommand& command,
                                const std::optional<RectI>& clip,
                                std::vector<RenderEvent>& events);

void render_text_command(PixelCanvas& canvas,
                         const TextCommand& command,
                         const std::optional<RectI>& clip,
                         std::vector<RenderEvent>& events);

} // namespace pluss::view::window::detail
