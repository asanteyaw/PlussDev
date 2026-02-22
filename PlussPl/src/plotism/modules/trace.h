#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "pluss/plotism/modules/axes.h"
#include "pluss/plotism/modules/style.h"

namespace pluss::plotism {

enum class Trace2DKind : std::uint8_t {
  Line = 0,
  Scatter = 1,
  Bar = 2,
  Hist = 3,
};

struct Trace2D {
  Trace2DKind kind{Trace2DKind::Line};
  std::string name{};
  std::vector<double> x{};
  std::vector<double> y{};
  std::vector<std::string> x_labels{};
  std::int64_t bins{0};
  Style style{};
};

struct Subplot2D {
  std::string title{};
  Axis x_axis{};
  Axis y_axis{};
  Legend legend{};
  Style style{};
  std::vector<Trace2D> traces{};
};

enum class Trace3DKind : std::uint8_t {
  Line3 = 0,
  Scatter3 = 1,
  Surface = 2,
};

struct Trace3D {
  Trace3DKind kind{Trace3DKind::Line3};
  std::string name{};
  std::vector<double> x{};
  std::vector<double> y{};
  std::vector<double> z{};
  Style style{};
};

} // namespace pluss::plotism
