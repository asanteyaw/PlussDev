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
};

struct Style {
  std::string color{};
  std::string line_style{"solid"};
  std::string marker{"none"};
  double line_width{1.5};
  double alpha{1.0};
};

struct Legend {
  bool visible{true};
  LegendLocation location{LegendLocation::Best};
  std::string title{};
};

} // namespace pluss::plotism
