#pragma once

#include <cstdint>
#include <string>

namespace pluss::plotism {

enum class LegendLocation : std::uint8_t {
  Best = 0,
  TopLeft = 1,
  TopRight = 2,
  BottomLeft = 3,
  BottomRight = 4,
  OutsideRight = 5,
};

enum class ColorbarLocation : std::uint8_t {
  Right = 0,
};

struct Style {
  std::string color{};
  std::string line_style{"solid"};
  std::string marker{"none"};
  double marker_size{0.0};
  double line_width{1.5};
  double alpha{1.0};
  std::string theme{};
  std::string font_family{};
};

struct Legend {
  bool visible{true};
  LegendLocation location{LegendLocation::Best};
  std::string title{};
};

struct Colorbar {
  bool visible{true};
  ColorbarLocation location{ColorbarLocation::Right};
  std::string title{};
};

} // namespace pluss::plotism
