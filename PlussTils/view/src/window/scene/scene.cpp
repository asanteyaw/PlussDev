#include "pluss/tils/view/window/scene.h"

#include <algorithm>
#include <cmath>
#include <type_traits>
#include <utility>

namespace pluss::view::window {

void Scene::clear_commands() {
  commands_.clear();
}

void Scene::add(DrawCommand command) {
  commands_.push_back(std::move(command));
}

const std::vector<DrawCommand>& Scene::commands() const noexcept {
  return commands_;
}

Scene scale_scene(const Scene& scene, const double scale) {
  if (!std::isfinite(scale) || scale <= 0.0 || std::abs(scale - 1.0) < 1e-9) {
    return scene;
  }

  const auto scalar = [scale](const int value) {
    return static_cast<int>(std::lround(static_cast<double>(value) * scale));
  };
  const auto positive = [&](const int value) {
    return value > 0 ? std::max(1, scalar(value)) : 0;
  };
  const auto point = [&](const PointI value) {
    return PointI{scalar(value.x), scalar(value.y)};
  };
  const auto rect = [&](const RectI value) {
    return RectI{scalar(value.x), scalar(value.y), positive(value.width), positive(value.height)};
  };

  Scene out;
  for (const auto& command : scene.commands()) {
    std::visit(
        [&](const auto& source) {
          using CommandT = std::decay_t<decltype(source)>;
          auto scaled = source;
          if constexpr (std::is_same_v<CommandT, FillRectCommand>) {
            scaled.rect = rect(source.rect);
          } else if constexpr (std::is_same_v<CommandT, FillRoundedRectCommand>) {
            scaled.rect = rect(source.rect);
            scaled.radius = positive(source.radius);
          } else if constexpr (std::is_same_v<CommandT, FillLinearGradientCommand>) {
            scaled.rect = rect(source.rect);
            scaled.radius = positive(source.radius);
          } else if constexpr (std::is_same_v<CommandT, StrokeRectCommand>) {
            scaled.rect = rect(source.rect);
            scaled.thickness = positive(source.thickness);
          } else if constexpr (std::is_same_v<CommandT, StrokeRoundedRectCommand>) {
            scaled.rect = rect(source.rect);
            scaled.radius = positive(source.radius);
            scaled.thickness = positive(source.thickness);
          } else if constexpr (std::is_same_v<CommandT, LineCommand>) {
            scaled.start = point(source.start);
            scaled.end = point(source.end);
            scaled.thickness = positive(source.thickness);
          } else if constexpr (std::is_same_v<CommandT, PolylineCommand>) {
            for (auto& item : scaled.points) {
              item = point(item);
            }
            scaled.thickness = positive(source.thickness);
          } else if constexpr (std::is_same_v<CommandT, FillPolygonCommand>) {
            for (auto& item : scaled.points) {
              item = point(item);
            }
          } else if constexpr (std::is_same_v<CommandT, MarkerCloudCommand>) {
            for (auto& item : scaled.points) {
              item = point(item);
            }
            scaled.radius = positive(source.radius);
          } else if constexpr (std::is_same_v<CommandT, SetClipRectCommand>) {
            scaled.rect = rect(source.rect);
          } else if constexpr (std::is_same_v<CommandT, TextCommand>) {
            scaled.anchor = point(source.anchor);
            scaled.pixel_size = positive(source.pixel_size);
          } else if constexpr (std::is_same_v<CommandT, TextCaretCommand>) {
            scaled.text_anchor = point(source.text_anchor);
            scaled.pixel_size = positive(source.pixel_size);
            scaled.fallback_width = scalar(source.fallback_width);
            scaled.max_x = scalar(source.max_x);
            scaled.y0 = scalar(source.y0);
            scaled.y1 = scalar(source.y1);
            scaled.thickness = positive(source.thickness);
          }
          out.add(std::move(scaled));
        },
        command);
  }
  return out;
}

} // namespace pluss::view::window
