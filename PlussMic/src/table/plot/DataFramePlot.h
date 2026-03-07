#pragma once

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string_view>

#include "table/DataFrame.h"
#include "pluss/pl.h"

namespace pluss::table {

namespace _ops {
enum class PlotDispatch : std::uint8_t;
struct PlotCall;
pluss::pl::Studio call(const class DataFramePlotAccessor& accessor, PlotDispatch dispatch, PlotCall args);
} // namespace _ops

using ScatterMatrixTriangle = pluss::pl::flash::PairPlotTriangle;

struct DataFramePlotOptions {
  std::optional<std::string> default_x_feature{std::nullopt};
  std::optional<std::string> default_hue_feature{std::nullopt};
  std::int64_t default_bins{20};
  std::optional<double> width{std::nullopt};
  std::optional<double> height{std::nullopt};
  bool use_tight_layout{false};
  double tight_layout_padding{6.0};
};

struct BarPlotOptions {
  bool ci_enabled{false};
  double ci_level{0.95};
  bool horizontal{false};
};

struct HeatmapPlotOptions {
  bool annotate_levels{false};
  bool scale_guide{true};
};

struct CorrHeatmapPlotOptions {
  bool annotate_levels{true};
  bool scale_guide{true};
};

struct ScatterMatrixPlotOptions {
  bool corner{false};
  pluss::pl::flash::PairPlotOptions pair{};
};

class DataFramePlotAccessor {
public:
  explicit DataFramePlotAccessor(DataFrame frame, DataFramePlotOptions options = {});

  pluss::pl::Studio line(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio line(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio step(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio step(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area_stacked(std::initializer_list<std::string_view> y_features,
                                 std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio scatter(std::string_view x_feature,
                            std::string_view y_feature,
                            std::optional<std::string_view> hue_feature = std::nullopt) const;
  pluss::pl::Studio bar(std::string_view y_feature,
                        std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio bar(std::string_view y_feature,
                        std::optional<std::string_view> x_feature,
                        const BarPlotOptions& options) const;
  pluss::pl::Studio bar(std::initializer_list<std::string_view> y_features,
                        std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio bar(std::initializer_list<std::string_view> y_features,
                        std::optional<std::string_view> x_feature,
                        const BarPlotOptions& options) const;
  pluss::pl::Studio bar_stacked(std::initializer_list<std::string_view> y_features,
                                std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio box(std::initializer_list<std::string_view> features = {}) const;
  pluss::pl::Studio violin(std::initializer_list<std::string_view> features = {}) const;
  pluss::pl::Studio violin(std::string_view value_feature, std::string_view group_feature) const;
  pluss::pl::Studio hist(std::int64_t bins = 20) const;
  pluss::pl::Studio hist(std::string_view feature, std::int64_t bins = 20) const;
  pluss::pl::Studio hist(std::initializer_list<std::string_view> features, std::int64_t bins = 20) const;
  pluss::pl::Studio density(std::string_view feature, std::int64_t points = 128) const;
  pluss::pl::Studio density(std::initializer_list<std::string_view> features, std::int64_t points = 128) const;
  pluss::pl::Studio pie(std::string_view value_feature,
                        std::optional<std::string_view> label_feature = std::nullopt) const;
  pluss::pl::Studio heatmap(std::initializer_list<std::string_view> features) const;
  pluss::pl::Studio heatmap(std::initializer_list<std::string_view> features,
                            const HeatmapPlotOptions& options) const;
  pluss::pl::Studio heatmap(std::string_view case_feature,
                            std::string_view feature_feature,
                            std::string_view value_feature,
                            DataFrame::GroupOp op = DataFrame::GroupOp::Mean) const;
  pluss::pl::Studio heatmap(std::string_view case_feature,
                            std::string_view feature_feature,
                            std::string_view value_feature,
                            DataFrame::GroupOp op,
                            const HeatmapPlotOptions& options) const;
  pluss::pl::Studio corr_heatmap(std::initializer_list<std::string_view> features = {}) const;
  pluss::pl::Studio corr_heatmap(std::initializer_list<std::string_view> features,
                                 const CorrHeatmapPlotOptions& options) const;
  pluss::pl::Studio scatter_matrix(std::initializer_list<std::string_view> features = {}) const;
  pluss::pl::Studio scatter_matrix(std::initializer_list<std::string_view> features,
                                   ScatterMatrixTriangle triangle) const;
  pluss::pl::Studio scatter_matrix(std::initializer_list<std::string_view> features,
                                   const ScatterMatrixPlotOptions& options) const;
  pluss::pl::Studio parallel(
      std::initializer_list<std::string_view> features = {},
      std::optional<std::string_view> class_feature = std::nullopt,
      const pluss::pl::flash::ParallelCoordinatesOptions& options = {}) const;
  pluss::pl::Studio andrews_curves(
      std::initializer_list<std::string_view> features = {},
      std::optional<std::string_view> class_feature = std::nullopt,
      const pluss::pl::flash::AndrewsCurvesOptions& options = {}) const;
  pluss::pl::Studio radviz(
      std::initializer_list<std::string_view> features = {},
      std::optional<std::string_view> class_feature = std::nullopt,
      const pluss::pl::flash::RadvizOptions& options = {}) const;
  pluss::pl::Studio lag(std::string_view feature,
                        const pluss::pl::flash::LagPlotOptions& options = {}) const;
  pluss::pl::Studio act(
      std::string_view feature,
      const pluss::pl::flash::AutocorrelationPlotOptions& options = {}) const;
  pluss::pl::Studio bootstrap(
      std::string_view feature,
      const pluss::pl::flash::BootstrapPlotOptions& options = {}) const;
  pluss::pl::Studio brief(std::initializer_list<std::string_view> features = {},
                          const pluss::pl::flash::SummaryOptions& options = {}) const;
  pluss::pl::Studio auto_plot(std::initializer_list<std::string_view> features = {}) const;

private:
  friend pluss::pl::Studio _ops::call(const DataFramePlotAccessor& accessor,
                                      _ops::PlotDispatch dispatch,
                                      _ops::PlotCall args);

  pluss::pl::Studio impl_line(std::initializer_list<std::string_view> y_features,
                              std::optional<std::string_view> x_feature) const;
  pluss::pl::Studio impl_step(std::initializer_list<std::string_view> y_features,
                              std::optional<std::string_view> x_feature) const;
  pluss::pl::Studio impl_area(std::initializer_list<std::string_view> y_features,
                              std::optional<std::string_view> x_feature) const;
  pluss::pl::Studio impl_area_stacked(std::initializer_list<std::string_view> y_features,
                                      std::optional<std::string_view> x_feature) const;
  pluss::pl::Studio impl_scatter(std::string_view x_feature,
                                 std::string_view y_feature,
                                 std::optional<std::string_view> hue_feature) const;
  pluss::pl::Studio impl_bar(std::initializer_list<std::string_view> y_features,
                             std::optional<std::string_view> x_feature) const;
  pluss::pl::Studio impl_bar(std::initializer_list<std::string_view> y_features,
                             std::optional<std::string_view> x_feature,
                             const BarPlotOptions& options) const;
  pluss::pl::Studio impl_bar_stacked(std::initializer_list<std::string_view> y_features,
                                     std::optional<std::string_view> x_feature) const;
  pluss::pl::Studio impl_box(std::initializer_list<std::string_view> features) const;
  pluss::pl::Studio impl_violin(std::initializer_list<std::string_view> features) const;
  pluss::pl::Studio impl_violin(std::string_view value_feature, std::string_view group_feature) const;
  pluss::pl::Studio impl_hist(std::string_view feature, std::int64_t bins) const;
  pluss::pl::Studio impl_hist(std::initializer_list<std::string_view> features, std::int64_t bins) const;
  pluss::pl::Studio impl_density(std::initializer_list<std::string_view> features, std::int64_t points) const;
  pluss::pl::Studio impl_pie(std::string_view value_feature, std::optional<std::string_view> label_feature) const;
  pluss::pl::Studio impl_heatmap(std::initializer_list<std::string_view> features) const;
  pluss::pl::Studio impl_heatmap(std::initializer_list<std::string_view> features,
                                 const HeatmapPlotOptions& options) const;
  pluss::pl::Studio impl_heatmap(std::string_view case_feature,
                                 std::string_view feature_feature,
                                 std::string_view value_feature,
                                 DataFrame::GroupOp op) const;
  pluss::pl::Studio impl_heatmap(std::string_view case_feature,
                                 std::string_view feature_feature,
                                 std::string_view value_feature,
                                 DataFrame::GroupOp op,
                                 const HeatmapPlotOptions& options) const;
  pluss::pl::Studio impl_corr_heatmap(std::initializer_list<std::string_view> features) const;
  pluss::pl::Studio impl_corr_heatmap(std::initializer_list<std::string_view> features,
                                      const CorrHeatmapPlotOptions& options) const;
  pluss::pl::Studio impl_scatter_matrix(std::initializer_list<std::string_view> features) const;
  pluss::pl::Studio impl_scatter_matrix(std::initializer_list<std::string_view> features,
                                        const ScatterMatrixPlotOptions& options) const;
  pluss::pl::Studio impl_parallel(
      std::initializer_list<std::string_view> features,
      std::optional<std::string_view> class_feature,
      const pluss::pl::flash::ParallelCoordinatesOptions& options) const;
  pluss::pl::Studio impl_andrews_curves(
      std::initializer_list<std::string_view> features,
      std::optional<std::string_view> class_feature,
      const pluss::pl::flash::AndrewsCurvesOptions& options) const;
  pluss::pl::Studio impl_radviz(std::initializer_list<std::string_view> features,
                                std::optional<std::string_view> class_feature,
                                const pluss::pl::flash::RadvizOptions& options) const;
  pluss::pl::Studio impl_lag(std::string_view feature,
                             const pluss::pl::flash::LagPlotOptions& options) const;
  pluss::pl::Studio impl_act(
      std::string_view feature,
      const pluss::pl::flash::AutocorrelationPlotOptions& options) const;
  pluss::pl::Studio impl_bootstrap(
      std::string_view feature,
      const pluss::pl::flash::BootstrapPlotOptions& options) const;
  pluss::pl::Studio impl_brief(std::initializer_list<std::string_view> features,
                               const pluss::pl::flash::SummaryOptions& options) const;
  pluss::pl::Studio impl_auto_plot(std::initializer_list<std::string_view> features) const;
  [[nodiscard]] std::optional<std::string_view> resolve_x_feature(std::optional<std::string_view> x_feature) const;
  [[nodiscard]] std::optional<std::string_view> resolve_hue_feature(std::optional<std::string_view> hue_feature) const;
  [[nodiscard]] std::int64_t resolve_bins(std::int64_t bins) const noexcept;
  void apply_figure_options(pluss::pl::Studio& figure) const;

  DataFrame frame_{};
  DataFramePlotOptions options_{};
};

class DataFramePlot3DAccessor {
public:
  explicit DataFramePlot3DAccessor(DataFrame frame, DataFramePlotOptions options = {});

  pluss::pl::Studio scatter(std::string_view x_feature,
                            std::string_view y_feature,
                            std::string_view z_feature,
                            std::optional<std::string_view> size_feature = std::nullopt,
                            std::optional<std::string_view> color_feature = std::nullopt) const;
  pluss::pl::Studio line(std::string_view x_feature,
                         std::string_view y_feature,
                         std::string_view z_feature) const;
  pluss::pl::Studio surface(std::string_view x_feature,
                            std::string_view y_feature,
                            std::string_view z_feature,
                            DataFrame::GroupOp op = DataFrame::GroupOp::Mean) const;
  pluss::pl::Studio wireframe(std::string_view x_feature,
                              std::string_view y_feature,
                              std::string_view z_feature,
                              DataFrame::GroupOp op = DataFrame::GroupOp::Mean) const;

private:
  void apply_figure_options(pluss::pl::Studio& figure) const;

  DataFrame frame_{};
  DataFramePlotOptions options_{};
};

} // namespace pluss::table
