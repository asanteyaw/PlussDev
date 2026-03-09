#pragma once

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "table/DataFrame.h"
#include "pluss/pl.h"

namespace pluss::table {

using ScatterMatrixTriangle = pluss::pl::flash::PairPlotTriangle;

enum class DateTimeLabelView : std::uint8_t {
  Full,
  Date,
  Time,
};

struct DataFramePlotOptions {
  std::optional<std::string> default_x_feature{std::nullopt};
  std::optional<std::string> default_hue_feature{std::nullopt};
  std::int64_t default_bins{20};
  DateTimeLabelView datetime_label_view{DateTimeLabelView::Full};
  std::int64_t max_labeled_ticks{14};
  std::optional<double> labeled_tick_rotation_deg{std::nullopt};
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
  bool borderless{false};
};

struct CorrHeatmapPlotOptions {
  bool annotate_levels{true};
  bool scale_guide{true};
  bool borderless{false};
};

struct ScatterMatrixPlotOptions {
  bool corner{false};
  pluss::pl::flash::PairPlotOptions pair{};
};

struct ParallelPlotOptions {
  std::vector<std::string> labels{};
  bool normalize{true};
  std::int64_t max_points{1200};
  double alpha{0.24};
  float trace_width{0.9F};
  bool show_grid{true};
  bool show_legend{true};
};

struct AndrewsCurvesPlotOptions {
  std::vector<std::string> labels{};
  bool normalize{true};
  std::int64_t points{220};
  std::int64_t max_curves{600};
  double alpha{0.28};
  float trace_width{0.9F};
  bool show_grid{true};
  bool show_legend{true};
};

struct RadvizPlotOptions {
  std::vector<std::string> labels{};
  bool normalize{true};
  std::int64_t max_points{3000};
  double marker_area{18.0};
  float trace_width{0.55F};
  bool show_grid{true};
  bool show_anchors{true};
  bool show_legend{true};
};

struct LagPlotOptions {
  std::int64_t lag{1};
  std::int64_t max_points{2500};
  double marker_area{16.0};
  float trace_width{0.55F};
};

struct ActPlotOptions {
  std::int64_t max_lag{40};
  bool show_confidence_band{true};
  double confidence_level{0.95};
  float trace_width{1.2F};
};

enum class BootstrapStatistic : std::uint8_t {
  Mean,
  Median,
  Std,
};

struct BootstrapPlotOptions {
  std::int64_t samples{300};
  std::int64_t sample_size{0};
  std::uint64_t seed{7};
  int bins{24};
  BootstrapStatistic statistic{BootstrapStatistic::Mean};
  float trace_width{0.85F};
};

struct Plot3DSceneOptions {
  std::optional<pluss::pl::flash::ScenePreset3D> preset{std::nullopt};
  std::optional<pluss::pl::flash::ThemePreset3D> theme{std::nullopt};
  std::optional<pluss::pl::flash::CameraPreset3D> camera_preset{std::nullopt};
  std::optional<pluss::pl::flash::Camera3DOptions> camera{std::nullopt};
  std::optional<pluss::pl::flash::Lighting3DOptions> lighting{std::nullopt};
  std::optional<pluss::pl::GridStyle> grid_style{std::nullopt};
};

struct Plot3DTraceOptions {
  std::optional<pluss::pl::flash::TracePreset3D> preset{std::nullopt};
  std::optional<pluss::pl::ShadeMode> shade{std::nullopt};
  std::optional<pluss::pl::LightingMode> lighting{std::nullopt};
  std::optional<pluss::pl::MaterialProfile> material{std::nullopt};
  std::optional<float> line_width{std::nullopt};
  std::optional<bool> transparent{std::nullopt};
};

struct Scatter3DPlotOptions {
  Plot3DSceneOptions scene{};
  Plot3DTraceOptions trace{};
  double default_marker_size{12.0};
};

struct Line3DPlotOptions {
  Plot3DSceneOptions scene{};
  Plot3DTraceOptions trace{};
};

struct Hist3DPlotOptions {
  std::int64_t bins_x{26};
  std::int64_t bins_y{20};
  bool normalize{false};
  bool smooth{false};
  std::int64_t smooth_passes{1};
  bool as_surface{false};
  bool as_wireframe{false};
  Plot3DSceneOptions scene{};
  Plot3DTraceOptions trace{};
};

struct Surface3DPlotOptions {
  Plot3DSceneOptions scene{};
  Plot3DTraceOptions trace{};
};

struct Wireframe3DPlotOptions {
  Plot3DSceneOptions scene{};
  Plot3DTraceOptions trace{};
};

struct BriefPlotOptions {
  std::string title{"table"};
  int precision{4};
  bool include_header{true};
  std::int64_t max_cases{10};
  bool transpose{false};
  bool include_case_index{true};
  std::int64_t features_per_table{0};
  std::int64_t tables_per_row{1};
  std::int64_t table_gap_spaces{4};
};

struct LinePlotOptions {
  double alpha{1.0};
  double line_width{1.6};
  std::optional<pluss::pl::LimitMethod> y_limit_method{pluss::pl::LimitMethod::Padded};
};

struct StepPlotOptions {
  double alpha{1.0};
  double line_width{1.5};
};

struct AreaPlotOptions {
  double alpha{0.88};
  double line_width{1.3};
};

struct AreaStackedPlotOptions {
  double alpha{0.7};
  double line_width{1.2};
};

struct ScatterPlotOptions {
  double alpha{0.92};
  double line_width{1.1};
};

struct BarStackedPlotOptions {
  double alpha{0.65};
  double line_width{1.0};
};

struct BoxPlotOptions {
  double alpha{0.95};
  double line_width{1.1};
};

struct ViolinPlotOptions {
  double alpha{0.92};
  double line_width{1.1};
  double width{0.8};
  double bw_adjust{1.0};
  double cut{2.0};
  pluss::pl::ViolinScale scale{pluss::pl::ViolinScale::Area};
  pluss::pl::ViolinInner inner{pluss::pl::ViolinInner::Box};
  pluss::pl::ViolinSide side{pluss::pl::ViolinSide::Full};
};

struct HistPlotOptions {
  std::int64_t bins{20};
  double alpha{0.95};
  double line_width{1.0};
};

struct DensityPlotOptions {
  std::int64_t points{128};
  double alpha{1.0};
  double line_width{1.5};
};

struct PiePlotOptions {
  bool show_label{true};
  bool show_percent{true};
  bool show_value{false};
  int precision{1};
  std::string separator{" | "};
};

struct AutoPlotOptions {};

class DataFramePlotAccessor {
public:
  explicit DataFramePlotAccessor(DataFrame frame, DataFramePlotOptions options = {});

  pluss::pl::Studio line(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio line(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio line(std::string_view y_feature,
                         std::optional<std::string_view> x_feature,
                         const LinePlotOptions& options) const;
  pluss::pl::Studio line(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const LinePlotOptions& options) const;
  pluss::pl::Studio step(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio step(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio step(std::string_view y_feature,
                         std::optional<std::string_view> x_feature,
                         const StepPlotOptions& options) const;
  pluss::pl::Studio step(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const StepPlotOptions& options) const;
  pluss::pl::Studio area(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area(std::string_view y_feature,
                         std::optional<std::string_view> x_feature,
                         const AreaPlotOptions& options) const;
  pluss::pl::Studio area(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const AreaPlotOptions& options) const;
  pluss::pl::Studio area_stacked(std::initializer_list<std::string_view> y_features,
                                 std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area_stacked(std::initializer_list<std::string_view> y_features,
                                 std::optional<std::string_view> x_feature,
                                 const AreaStackedPlotOptions& options) const;
  pluss::pl::Studio scatter(std::string_view x_feature,
                            std::string_view y_feature,
                            std::optional<std::string_view> hue_feature = std::nullopt) const;
  pluss::pl::Studio scatter(std::string_view x_feature,
                            std::string_view y_feature,
                            std::optional<std::string_view> hue_feature,
                            const ScatterPlotOptions& options) const;
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
  pluss::pl::Studio bar_stacked(std::initializer_list<std::string_view> y_features,
                                std::optional<std::string_view> x_feature,
                                const BarStackedPlotOptions& options) const;
  pluss::pl::Studio box(std::initializer_list<std::string_view> features = {}) const;
  pluss::pl::Studio box(std::initializer_list<std::string_view> features,
                        const BoxPlotOptions& options) const;
  pluss::pl::Studio violin(std::initializer_list<std::string_view> features = {}) const;
  pluss::pl::Studio violin(std::initializer_list<std::string_view> features,
                           const ViolinPlotOptions& options) const;
  pluss::pl::Studio violin(std::string_view value_feature, std::string_view group_feature) const;
  pluss::pl::Studio violin(std::string_view value_feature,
                           std::string_view group_feature,
                           const ViolinPlotOptions& options) const;
  pluss::pl::Studio hist(std::int64_t bins = 20) const;
  pluss::pl::Studio hist(std::string_view feature, std::int64_t bins = 20) const;
  pluss::pl::Studio hist(std::initializer_list<std::string_view> features, std::int64_t bins = 20) const;
  pluss::pl::Studio hist(const HistPlotOptions& options) const;
  pluss::pl::Studio hist(std::string_view feature, const HistPlotOptions& options) const;
  pluss::pl::Studio hist(std::initializer_list<std::string_view> features, const HistPlotOptions& options) const;
  pluss::pl::Studio density(std::string_view feature, std::int64_t points = 128) const;
  pluss::pl::Studio density(std::initializer_list<std::string_view> features, std::int64_t points = 128) const;
  pluss::pl::Studio density(std::string_view feature, const DensityPlotOptions& options) const;
  pluss::pl::Studio density(std::initializer_list<std::string_view> features, const DensityPlotOptions& options) const;
  pluss::pl::Studio pie(std::string_view value_feature,
                        std::optional<std::string_view> label_feature = std::nullopt) const;
  pluss::pl::Studio pie(std::string_view value_feature,
                        std::optional<std::string_view> label_feature,
                        const PiePlotOptions& options) const;
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
      const ParallelPlotOptions& options = {}) const;
  pluss::pl::Studio andrews_curves(
      std::initializer_list<std::string_view> features = {},
      std::optional<std::string_view> class_feature = std::nullopt,
      const AndrewsCurvesPlotOptions& options = {}) const;
  pluss::pl::Studio radviz(
      std::initializer_list<std::string_view> features = {},
      std::optional<std::string_view> class_feature = std::nullopt,
      const RadvizPlotOptions& options = {}) const;
  pluss::pl::Studio lag(std::string_view feature,
                        const LagPlotOptions& options = {}) const;
  pluss::pl::Studio act(
      std::string_view feature,
      const ActPlotOptions& options = {}) const;
  pluss::pl::Studio bootstrap(
      std::string_view feature,
      const BootstrapPlotOptions& options = {}) const;
  pluss::pl::Studio brief(std::initializer_list<std::string_view> features = {},
                          const BriefPlotOptions& options = {}) const;
  pluss::pl::Studio& brief(pluss::pl::Studio& figure,
                           std::initializer_list<std::string_view> features = {},
                           const BriefPlotOptions& options = {}) const;
  pluss::pl::Studio auto_plot(std::initializer_list<std::string_view> features = {}) const;
  pluss::pl::Studio auto_plot(std::initializer_list<std::string_view> features,
                              const AutoPlotOptions& options) const;

private:
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
  pluss::pl::Studio impl_pie(std::string_view value_feature,
                             std::optional<std::string_view> label_feature,
                             const PiePlotOptions& options) const;
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
      const ParallelPlotOptions& options) const;
  pluss::pl::Studio impl_andrews_curves(
      std::initializer_list<std::string_view> features,
      std::optional<std::string_view> class_feature,
      const AndrewsCurvesPlotOptions& options) const;
  pluss::pl::Studio impl_radviz(std::initializer_list<std::string_view> features,
                                std::optional<std::string_view> class_feature,
                                const RadvizPlotOptions& options) const;
  pluss::pl::Studio impl_lag(std::string_view feature,
                             const LagPlotOptions& options) const;
  pluss::pl::Studio impl_act(
      std::string_view feature,
      const ActPlotOptions& options) const;
  pluss::pl::Studio impl_bootstrap(
      std::string_view feature,
      const BootstrapPlotOptions& options) const;
  pluss::pl::Studio impl_brief(std::initializer_list<std::string_view> features,
                               const BriefPlotOptions& options) const;
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
                            std::optional<std::string_view> color_feature = std::nullopt,
                            const Scatter3DPlotOptions& options = {}) const;
  pluss::pl::Studio line(std::string_view x_feature,
                         std::string_view y_feature,
                         std::string_view z_feature,
                         const Line3DPlotOptions& options = {}) const;
  pluss::pl::Studio hist(std::string_view x_feature,
                         std::string_view y_feature,
                         const Hist3DPlotOptions& options = {}) const;
  pluss::pl::Studio surface(std::string_view x_feature,
                            std::string_view y_feature,
                            std::string_view z_feature,
                            DataFrame::GroupOp op = DataFrame::GroupOp::Mean,
                            const Surface3DPlotOptions& options = {}) const;
  pluss::pl::Studio wireframe(std::string_view x_feature,
                              std::string_view y_feature,
                              std::string_view z_feature,
                              DataFrame::GroupOp op = DataFrame::GroupOp::Mean,
                              const Wireframe3DPlotOptions& options = {}) const;

private:
  void apply_figure_options(pluss::pl::Studio& figure) const;

  DataFrame frame_{};
  DataFramePlotOptions options_{};
};

} // namespace pluss::table
