#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "MSharp/RangeView.h"
#include "table/DataFrame.h"
#include "pluss/pl.h"

namespace pluss::table {

enum class AtlasDepthStatistic : std::uint8_t {
  Mean,
  Median,
  Min,
  Max,
};

enum class AtlasFrameSelectionMode : std::uint8_t {
  Overlay,
  Facet,
};

enum class AtlasDepthOrder : std::uint8_t {
  Ascending,
  Descending,
};

struct AtlasDepthAxisOptions {
  AtlasDepthOrder order{AtlasDepthOrder::Ascending};
  std::int64_t step{1};
};

struct AtlasFramesPlotOptions {
  AtlasFrameSelectionMode mode{AtlasFrameSelectionMode::Overlay};
  bool show_grid{true};
  std::int64_t rows{0};
  std::int64_t cols{0};
  double grid_gap_x{16.0};
  double grid_gap_y{16.0};
  float line_width{1.5F};
};

struct AtlasDepthPlotOptions {
  AtlasDepthStatistic center{AtlasDepthStatistic::Mean};
  std::optional<AtlasDepthStatistic> lower{AtlasDepthStatistic::Min};
  std::optional<AtlasDepthStatistic> upper{AtlasDepthStatistic::Max};
  bool show_grid{true};
  float center_trace_width{1.5F};
  float band_trace_width{1.0F};
};

struct AtlasDepthComparePlotOptions {
  AtlasDepthStatistic statistic{AtlasDepthStatistic::Mean};
  bool show_grid{true};
  float line_width{1.5F};
  std::int64_t max_features{0};
};

struct AtlasDepthFacetPlotOptions {
  AtlasDepthStatistic statistic{AtlasDepthStatistic::Mean};
  bool show_grid{true};
  std::int64_t rows{0};
  std::int64_t cols{0};
  double grid_gap_x{16.0};
  double grid_gap_y{16.0};
  std::int64_t max_features{0};
};

struct AtlasDepthRollingPlotOptions {
  AtlasDepthStatistic statistic{AtlasDepthStatistic::Mean};
  std::int64_t window{3};
  bool show_raw{true};
  bool show_grid{true};
  float raw_line_width{1.0F};
  float rolling_line_width{1.8F};
};

struct AtlasPlotOptions {
  DataFramePlotOptions figure{};
  AtlasFramesPlotOptions frames{};
  AtlasDepthAxisOptions depth_axis{};
  Surface3DPlotOptions surface{};
  Wireframe3DPlotOptions wireframe{};
  Scatter3DPlotOptions cloud3{};
  AtlasDepthPlotOptions depth{};
  AtlasDepthComparePlotOptions depth_compare{};
  AtlasDepthFacetPlotOptions depth_facet{};
  AtlasDepthRollingPlotOptions depth_rolling{};
  pluss::pl::flash::TriplotOptions triplot{};
};

class AtlasPlotAccessor {
public:
  AtlasPlotAccessor(std::vector<DataFrame> frames,
                    std::optional<Thread> depth_labels = std::nullopt,
                    AtlasPlotOptions options = {});

  DataFramePlotAccessor frames(std::int64_t frame_index = 0) const;
  pluss::pl::Studio frames(pluss::msharp::RangeView::Int frame_indices,
                           std::string_view y_feature,
                           std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio frames(pluss::msharp::RangeView::Int frame_indices,
                           std::string_view y_feature,
                           std::optional<std::string_view> x_feature,
                           const AtlasFramesPlotOptions& options) const;

  pluss::pl::Studio surface(std::string_view feature) const;
  pluss::pl::Studio surface(std::string_view feature, const Surface3DPlotOptions& options) const;
  pluss::pl::Studio wireframe(std::string_view feature) const;
  pluss::pl::Studio wireframe(std::string_view feature, const Wireframe3DPlotOptions& options) const;
  pluss::pl::Studio cloud3(std::string_view x_feature, std::string_view y_feature, std::string_view z_feature) const;
  pluss::pl::Studio cloud3(std::string_view x_feature,
                           std::string_view y_feature,
                           std::string_view z_feature,
                           const Scatter3DPlotOptions& options) const;
  pluss::pl::Studio depth(std::string_view feature) const;
  pluss::pl::Studio depth(std::string_view feature, const AtlasDepthPlotOptions& options) const;
  pluss::pl::Studio depth_compare(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  pluss::pl::Studio depth_compare(pluss::msharp::RangeView::Of<std::string_view> features,
                                  const AtlasDepthComparePlotOptions& options) const;
  pluss::pl::Studio depth_facet(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  pluss::pl::Studio depth_facet(pluss::msharp::RangeView::Of<std::string_view> features,
                                const AtlasDepthFacetPlotOptions& options) const;
  pluss::pl::Studio depth_rolling(std::string_view feature) const;
  pluss::pl::Studio depth_rolling(std::string_view feature, const AtlasDepthRollingPlotOptions& options) const;
  pluss::pl::Studio triplot() const;
  pluss::pl::Studio triplot(const pluss::pl::flash::TriplotOptions& options) const;
  pluss::pl::Studio triplot(pluss::msharp::RangeView::Of<std::string_view> features) const;
  pluss::pl::Studio triplot(pluss::msharp::RangeView::Of<std::string_view> features,
                            const pluss::pl::flash::TriplotOptions& options) const;

private:
  void apply_figure_options(pluss::pl::Studio& figure) const;

  std::vector<DataFrame> frames_{};
  std::optional<Thread> depth_labels_{std::nullopt};
  AtlasPlotOptions options_{};
};

} // namespace pluss::table
