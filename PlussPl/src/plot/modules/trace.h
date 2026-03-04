#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "plot/modules/axes.h"
#include "plot/modules/camera_3d.h"
#include "plot/modules/style.h"

namespace pluss::plotism {

enum class Trace2DKind : std::uint8_t {
  Line = 0,
  Scatter = 1,
  Bar = 2,
  Hist = 3,
  Step = 4,
  Area = 5,
  Band = 6,
  Polygon = 7,
  Heatmap = 8,
  Annotation = 9,
  ErrorBar = 10,
};

struct Trace2D {
  Trace2DKind kind{Trace2DKind::Line};
  std::string name{};
  std::vector<double> x{};
  std::vector<double> x2{};
  std::vector<double> y{};
  std::vector<double> y2{};
  std::vector<double> values{};
  std::vector<std::string> x_labels{};
  std::vector<std::string> y_labels{};
  std::string text{};
  std::int64_t bins{0};
  std::int64_t rows{0};
  std::int64_t cols{0};
  Style style{};
};

struct Subplot2D {
  std::string title{};
  XAxis x_axis{};
  YAxis y_axis{};
  Legend legend{};
  std::optional<Colorbar> colorbar{};
  Style style{};
  bool polar_projection{false};
  std::vector<Trace2D> traces{};
};

enum class Trace3DKind : std::uint8_t {
  Line3 = 0,
  Scatter3 = 1,
  Surface = 2,
  Patch3 = 3,
};

enum class SurfaceCurtainMode : std::uint8_t {
  None = 0,
  Boundary = 1,
  Rows = 2,
};

struct Trace3D {
  Trace3DKind kind{Trace3DKind::Line3};
  std::string name{};
  std::vector<double> x{};
  std::vector<double> y{};
  std::vector<double> z{};
  std::int64_t rows{0};
  std::int64_t cols{0};
  bool filled{true};
  bool wireframe{true};
  bool wireframe_rows{true};
  bool wireframe_cols{true};
  bool contours{false};
  bool base_contours{false};
  bool lighted{false};
  SurfaceCurtainMode curtain_mode{SurfaceCurtainMode::None};
  double light_azimuth_deg{45.0};
  double light_elevation_deg{30.0};
  std::int32_t contour_stride{3};
  Style style{};
};

enum class Trace4DKind : std::uint8_t {
  Line4 = 0,
  Scatter4 = 1,
};

struct Trace4D {
  Trace4DKind kind{Trace4DKind::Scatter4};
  std::string name{};
  std::vector<double> x{};
  std::vector<double> y{};
  std::vector<double> z{};
  std::vector<double> w{};
  Style style{};
};

} // namespace pluss::plotism
