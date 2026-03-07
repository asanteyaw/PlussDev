#pragma once

#include <string>
#include <variant>
#include <vector>

#include "pluss/view/window/types.h"

namespace pluss::view::window {

struct ClearCommand {
  Color color{Color::rgba(250, 250, 250, 255)};
};

struct FillRectCommand {
  RectI rect{};
  Color color{Color::rgba(32, 32, 32, 255)};
};

struct FillRoundedRectCommand {
  RectI rect{};
  int radius{0};
  Color color{Color::rgba(32, 32, 32, 255)};
};

struct FillLinearGradientCommand {
  RectI rect{};
  Color start_color{Color::rgba(255, 255, 255, 255)};
  Color end_color{Color::rgba(240, 240, 240, 255)};
  bool horizontal{false};
  int radius{0};
};

struct StrokeRectCommand {
  RectI rect{};
  int thickness{1};
  Color color{Color::rgba(32, 32, 32, 255)};
};

struct StrokeRoundedRectCommand {
  RectI rect{};
  int radius{0};
  int thickness{1};
  Color color{Color::rgba(32, 32, 32, 255)};
};

struct LineCommand {
  PointI start{};
  PointI end{};
  int thickness{1};
  Color color{Color::rgba(32, 32, 32, 255)};
};

struct PolylineCommand {
  std::vector<PointI> points{};
  int thickness{1};
  Color color{Color::rgba(32, 32, 32, 255)};
};

struct FillPolygonCommand {
  std::vector<PointI> points{};
  Color color{Color::rgba(32, 32, 32, 255)};
};

struct MarkerCloudCommand {
  std::vector<PointI> points{};
  int radius{2};
  Color color{Color::rgba(28, 126, 214, 255)};
};

struct SetClipRectCommand {
  RectI rect{};
};

struct ClearClipRectCommand {};

struct TextCommand {
  PointI anchor{};
  std::string text{};
  int pixel_size{12};
  Color color{Color::rgba(32, 32, 32, 255)};
  int font_weight{400};
  std::string font_family{};
};

using DrawCommand = std::variant<ClearCommand,
                                 FillRectCommand,
                                 FillRoundedRectCommand,
                                 FillLinearGradientCommand,
                                 StrokeRectCommand,
                                 StrokeRoundedRectCommand,
                                 LineCommand,
                                 PolylineCommand,
                                 FillPolygonCommand,
                                 MarkerCloudCommand,
                                 SetClipRectCommand,
                                 ClearClipRectCommand,
                                 TextCommand>;

class Scene {
 public:
  Scene() = default;

  void clear_commands();
  void add(DrawCommand command);

  [[nodiscard]] const std::vector<DrawCommand>& commands() const noexcept;

 private:
  std::vector<DrawCommand> commands_{};
};

} // namespace pluss::view::window
