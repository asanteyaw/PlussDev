#pragma once

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string_view>

#include "table/DataFrame.h"
#include "pluss/pl.h"

namespace pluss::table {

class DataFramePlotAccessor;

namespace _ops {

enum class PlotDispatch : std::uint8_t {
  Line,
  Step,
  Area,
  AreaStacked,
  Scatter,
  Bar,
  BarStacked,
  Box,
  Hist,
  Heatmap,
  HeatmapPivot,
  CorrHeatmap,
  ScatterMatrix,
  AutoPlot,
};

struct PlotCall {
  std::initializer_list<std::string_view> features{};
  std::optional<std::string_view> x_feature{std::nullopt};
  std::optional<std::string_view> hue_feature{std::nullopt};
  std::int64_t bins{20};
  DataFrame::GroupOp group_op{DataFrame::GroupOp::Mean};
};

pluss::pl::Studio call(const DataFramePlotAccessor& accessor, PlotDispatch dispatch, PlotCall args = {});

} // namespace _ops

} // namespace pluss::table
