#pragma once

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "MSharp/RangeView.h"
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
  std::optional<double> x_tick_rotation_deg{std::nullopt};
  std::optional<double> y_tick_rotation_deg{std::nullopt};
  std::optional<double> x_label_rotation_deg{std::nullopt};
  std::optional<double> y_label_rotation_deg{std::nullopt};
  std::optional<pluss::pl::TickFormat> x_tick_format{std::nullopt};
  std::optional<pluss::pl::TickFormat> y_tick_format{std::nullopt};
  std::optional<double> x_tick_label_gap_points{std::nullopt};
  std::optional<double> y_tick_label_gap_points{std::nullopt};
  std::optional<double> x_label_gap_points{std::nullopt};
  std::optional<double> y_label_gap_points{std::nullopt};
  // Legacy single-axis rotation control (x-axis). Prefer x_tick_rotation_deg.
  std::optional<double> labeled_tick_rotation_deg{std::nullopt};
  std::optional<double> width{std::nullopt};
  std::optional<double> height{std::nullopt};
  bool use_tight_layout{false};
  double tight_layout_padding{6.0};
};

enum class XYAlignMode : std::uint8_t {
  Strict = 0,
  Inner = 1,
  PadScalar = 2,
};

enum class XYAlignBy : std::uint8_t {
  Auto = 0,
  Label = 1,
  Pos = 2,
};

struct XYAlignOptions {
  XYAlignMode mode{XYAlignMode::Strict};
  XYAlignBy align_by{XYAlignBy::Auto};
  double pad_scalar{0.0};
};

struct BarPlotOptions {
  bool ci_enabled{false};
  double ci_level{0.95};
  bool horizontal{false};
  XYAlignOptions align{};
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
  XYAlignOptions align{XYAlignMode::Inner, XYAlignBy::Auto, 0.0};
};

struct Line3DPlotOptions {
  Plot3DSceneOptions scene{};
  Plot3DTraceOptions trace{};
  XYAlignOptions align{XYAlignMode::Inner, XYAlignBy::Auto, 0.0};
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
  XYAlignOptions align{XYAlignMode::Inner, XYAlignBy::Auto, 0.0};
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
  XYAlignOptions align{};
};

struct StepPlotOptions {
  double alpha{1.0};
  double line_width{1.5};
  XYAlignOptions align{};
};

struct AreaPlotOptions {
  double alpha{0.88};
  double line_width{1.3};
  XYAlignOptions align{};
};

struct AreaStackedPlotOptions {
  double alpha{0.7};
  double line_width{1.2};
};

struct ScatterPlotOptions {
  double alpha{0.92};
  double line_width{1.1};
  bool fit_line{false};
  std::int64_t fit_order{1};
  bool fit_ci{false};
  double fit_confidence_level{0.95};
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

class SlatePlotAccessor {
public:
  explicit SlatePlotAccessor(Slate value, DataFramePlotOptions options = {});

  pluss::pl::Studio line() const;
  pluss::pl::Studio line(const Slate& x) const;
  pluss::pl::Studio line(const LinePlotOptions& options) const;
  pluss::pl::Studio line(const Slate& x, const LinePlotOptions& options) const;

  pluss::pl::Studio step() const;
  pluss::pl::Studio step(const Slate& x) const;
  pluss::pl::Studio step(const StepPlotOptions& options) const;
  pluss::pl::Studio step(const Slate& x, const StepPlotOptions& options) const;

  pluss::pl::Studio area() const;
  pluss::pl::Studio area(const Slate& x) const;
  pluss::pl::Studio area(const AreaPlotOptions& options) const;
  pluss::pl::Studio area(const Slate& x, const AreaPlotOptions& options) const;

  pluss::pl::Studio scatter(const Slate& x) const;
  pluss::pl::Studio scatter(const Slate& x, const ScatterPlotOptions& options) const;
  pluss::pl::Studio scatter(const Slate& x, const Slate& hue) const;
  pluss::pl::Studio scatter(const Slate& x, const Slate& hue, const ScatterPlotOptions& options) const;

  pluss::pl::Studio bar() const;
  pluss::pl::Studio bar(const Slate& x) const;
  pluss::pl::Studio bar(const BarPlotOptions& options) const;
  pluss::pl::Studio bar(const Slate& x, const BarPlotOptions& options) const;

  pluss::pl::Studio box(const BoxPlotOptions& options = {}) const;
  pluss::pl::Studio violin(const ViolinPlotOptions& options = {}) const;
  pluss::pl::Studio violin(const Slate& group) const;
  pluss::pl::Studio violin(const Slate& group, const ViolinPlotOptions& options) const;

  pluss::pl::Studio hist(std::int64_t bins = 20) const;
  pluss::pl::Studio hist(const HistPlotOptions& options) const;

  pluss::pl::Studio density(std::int64_t points = 128) const;
  pluss::pl::Studio density(const DensityPlotOptions& options) const;

  pluss::pl::Studio pie() const;
  pluss::pl::Studio pie(const PiePlotOptions& options) const;
  pluss::pl::Studio pie(const Slate& labels) const;
  pluss::pl::Studio pie(const Slate& labels, const PiePlotOptions& options) const;

  pluss::pl::Studio lag(const LagPlotOptions& options = {}) const;
  pluss::pl::Studio act(const ActPlotOptions& options = {}) const;
  pluss::pl::Studio bootstrap(const BootstrapPlotOptions& options = {}) const;
  pluss::pl::Studio brief(const BriefPlotOptions& options = {}) const;
  pluss::pl::Studio& brief(pluss::pl::Studio& figure,
                           const BriefPlotOptions& options = {}) const;
  pluss::pl::Studio auto_plot() const;
  pluss::pl::Studio auto_plot(const AutoPlotOptions& options) const;

private:
  Slate base_{};
  DataFramePlotOptions options_{};
};

class DataFramePlotAccessor {
public:
  explicit DataFramePlotAccessor(DataFrame frame, DataFramePlotOptions options = {});

  pluss::pl::Studio line(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio line(pluss::msharp::RangeView::Of<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio line(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const {
    return line(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature);
  }
  pluss::pl::Studio line(std::string_view y_feature,
                         std::optional<std::string_view> x_feature,
                         const LinePlotOptions& options) const;
  pluss::pl::Studio line(pluss::msharp::RangeView::Of<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const LinePlotOptions& options) const;
  pluss::pl::Studio line(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const LinePlotOptions& options) const {
    return line(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options);
  }
  pluss::pl::Studio step(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio step(pluss::msharp::RangeView::Of<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio step(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const {
    return step(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature);
  }
  pluss::pl::Studio step(std::string_view y_feature,
                         std::optional<std::string_view> x_feature,
                         const StepPlotOptions& options) const;
  pluss::pl::Studio step(pluss::msharp::RangeView::Of<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const StepPlotOptions& options) const;
  pluss::pl::Studio step(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const StepPlotOptions& options) const {
    return step(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options);
  }
  pluss::pl::Studio area(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area(pluss::msharp::RangeView::Of<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const {
    return area(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature);
  }
  pluss::pl::Studio area(std::string_view y_feature,
                         std::optional<std::string_view> x_feature,
                         const AreaPlotOptions& options) const;
  pluss::pl::Studio area(pluss::msharp::RangeView::Of<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const AreaPlotOptions& options) const;
  pluss::pl::Studio area(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const AreaPlotOptions& options) const {
    return area(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options);
  }
  pluss::pl::Studio area_stacked(pluss::msharp::RangeView::Of<std::string_view> y_features,
                                 std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area_stacked(std::initializer_list<std::string_view> y_features,
                                 std::optional<std::string_view> x_feature = std::nullopt) const {
    return area_stacked(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature);
  }
  pluss::pl::Studio area_stacked(pluss::msharp::RangeView::Of<std::string_view> y_features,
                                 std::optional<std::string_view> x_feature,
                                 const AreaStackedPlotOptions& options) const;
  pluss::pl::Studio area_stacked(std::initializer_list<std::string_view> y_features,
                                 std::optional<std::string_view> x_feature,
                                 const AreaStackedPlotOptions& options) const {
    return area_stacked(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options);
  }
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
  pluss::pl::Studio bar(pluss::msharp::RangeView::Of<std::string_view> y_features,
                        std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio bar(std::initializer_list<std::string_view> y_features,
                        std::optional<std::string_view> x_feature = std::nullopt) const {
    return bar(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature);
  }
  pluss::pl::Studio bar(pluss::msharp::RangeView::Of<std::string_view> y_features,
                        std::optional<std::string_view> x_feature,
                        const BarPlotOptions& options) const;
  pluss::pl::Studio bar(std::initializer_list<std::string_view> y_features,
                        std::optional<std::string_view> x_feature,
                        const BarPlotOptions& options) const {
    return bar(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options);
  }
  pluss::pl::Studio bar_stacked(pluss::msharp::RangeView::Of<std::string_view> y_features,
                                std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio bar_stacked(std::initializer_list<std::string_view> y_features,
                                std::optional<std::string_view> x_feature = std::nullopt) const {
    return bar_stacked(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature);
  }
  pluss::pl::Studio bar_stacked(pluss::msharp::RangeView::Of<std::string_view> y_features,
                                std::optional<std::string_view> x_feature,
                                const BarStackedPlotOptions& options) const;
  pluss::pl::Studio bar_stacked(std::initializer_list<std::string_view> y_features,
                                std::optional<std::string_view> x_feature,
                                const BarStackedPlotOptions& options) const {
    return bar_stacked(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options);
  }
  pluss::pl::Studio box(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  pluss::pl::Studio box(std::initializer_list<std::string_view> features) const {
    return box(pluss::msharp::RangeView::Of<std::string_view>(features));
  }
  pluss::pl::Studio box(pluss::msharp::RangeView::Of<std::string_view> features,
                        const BoxPlotOptions& options) const;
  pluss::pl::Studio box(std::initializer_list<std::string_view> features,
                        const BoxPlotOptions& options) const {
    return box(pluss::msharp::RangeView::Of<std::string_view>(features), options);
  }
  pluss::pl::Studio violin(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  pluss::pl::Studio violin(std::initializer_list<std::string_view> features) const {
    return violin(pluss::msharp::RangeView::Of<std::string_view>(features));
  }
  pluss::pl::Studio violin(pluss::msharp::RangeView::Of<std::string_view> features,
                           const ViolinPlotOptions& options) const;
  pluss::pl::Studio violin(std::initializer_list<std::string_view> features,
                           const ViolinPlotOptions& options) const {
    return violin(pluss::msharp::RangeView::Of<std::string_view>(features), options);
  }
  pluss::pl::Studio violin(std::string_view value_feature, std::string_view group_feature) const;
  pluss::pl::Studio violin(std::string_view value_feature,
                           std::string_view group_feature,
                           const ViolinPlotOptions& options) const;
  pluss::pl::Studio hist(std::int64_t bins = 20) const;
  pluss::pl::Studio hist(std::string_view feature, std::int64_t bins = 20) const;
  pluss::pl::Studio hist(pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t bins = 20) const;
  pluss::pl::Studio hist(std::initializer_list<std::string_view> features, std::int64_t bins = 20) const {
    return hist(pluss::msharp::RangeView::Of<std::string_view>(features), bins);
  }
  pluss::pl::Studio hist(const HistPlotOptions& options) const;
  pluss::pl::Studio hist(std::string_view feature, const HistPlotOptions& options) const;
  pluss::pl::Studio hist(pluss::msharp::RangeView::Of<std::string_view> features, const HistPlotOptions& options) const;
  pluss::pl::Studio hist(std::initializer_list<std::string_view> features, const HistPlotOptions& options) const {
    return hist(pluss::msharp::RangeView::Of<std::string_view>(features), options);
  }
  pluss::pl::Studio density(std::string_view feature, std::int64_t points = 128) const;
  pluss::pl::Studio density(pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t points = 128) const;
  pluss::pl::Studio density(std::initializer_list<std::string_view> features, std::int64_t points = 128) const {
    return density(pluss::msharp::RangeView::Of<std::string_view>(features), points);
  }
  pluss::pl::Studio density(std::string_view feature, const DensityPlotOptions& options) const;
  pluss::pl::Studio density(pluss::msharp::RangeView::Of<std::string_view> features, const DensityPlotOptions& options) const;
  pluss::pl::Studio density(std::initializer_list<std::string_view> features,
                            const DensityPlotOptions& options) const {
    return density(pluss::msharp::RangeView::Of<std::string_view>(features), options);
  }
  pluss::pl::Studio pie(std::string_view value_feature,
                        std::optional<std::string_view> label_feature = std::nullopt) const;
  pluss::pl::Studio pie(std::string_view value_feature,
                        std::optional<std::string_view> label_feature,
                        const PiePlotOptions& options) const;
  pluss::pl::Studio heatmap(pluss::msharp::RangeView::Of<std::string_view> features) const;
  pluss::pl::Studio heatmap(pluss::msharp::RangeView::Of<std::string_view> features,
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
  pluss::pl::Studio corr_heatmap(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  pluss::pl::Studio corr_heatmap(pluss::msharp::RangeView::Of<std::string_view> features,
                                 const CorrHeatmapPlotOptions& options) const;
  pluss::pl::Studio scatter_matrix(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  pluss::pl::Studio scatter_matrix(pluss::msharp::RangeView::Of<std::string_view> features,
                                   ScatterMatrixTriangle triangle) const;
  pluss::pl::Studio scatter_matrix(pluss::msharp::RangeView::Of<std::string_view> features,
                                   const ScatterMatrixPlotOptions& options) const;
  pluss::pl::Studio parallel(
      pluss::msharp::RangeView::Of<std::string_view> features = {},
      std::optional<std::string_view> class_feature = std::nullopt,
      const ParallelPlotOptions& options = {}) const;
  pluss::pl::Studio andrews_curves(
      pluss::msharp::RangeView::Of<std::string_view> features = {},
      std::optional<std::string_view> class_feature = std::nullopt,
      const AndrewsCurvesPlotOptions& options = {}) const;
  pluss::pl::Studio radviz(
      pluss::msharp::RangeView::Of<std::string_view> features = {},
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
  pluss::pl::Studio brief(pluss::msharp::RangeView::Of<std::string_view> features = {},
                          const BriefPlotOptions& options = {}) const;
  pluss::pl::Studio& brief(pluss::pl::Studio& figure,
                           pluss::msharp::RangeView::Of<std::string_view> features = {},
                           const BriefPlotOptions& options = {}) const;
  pluss::pl::Studio auto_plot(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  pluss::pl::Studio auto_plot(pluss::msharp::RangeView::Of<std::string_view> features,
                              const AutoPlotOptions& options) const;

private:
  friend class SlatePlotAccessor;

  pluss::pl::Studio impl_line(pluss::msharp::RangeView::Of<std::string_view> y_features,
                              std::optional<std::string_view> x_feature) const;
  pluss::pl::Studio impl_step(pluss::msharp::RangeView::Of<std::string_view> y_features,
                              std::optional<std::string_view> x_feature) const;
  pluss::pl::Studio impl_area(pluss::msharp::RangeView::Of<std::string_view> y_features,
                              std::optional<std::string_view> x_feature) const;
  pluss::pl::Studio impl_area_stacked(pluss::msharp::RangeView::Of<std::string_view> y_features,
                                      std::optional<std::string_view> x_feature) const;
  pluss::pl::Studio impl_scatter(std::string_view x_feature,
                                 std::string_view y_feature,
                                 std::optional<std::string_view> hue_feature) const;
  pluss::pl::Studio impl_bar(pluss::msharp::RangeView::Of<std::string_view> y_features,
                             std::optional<std::string_view> x_feature) const;
  pluss::pl::Studio impl_bar(pluss::msharp::RangeView::Of<std::string_view> y_features,
                             std::optional<std::string_view> x_feature,
                             const BarPlotOptions& options) const;
  pluss::pl::Studio impl_bar_stacked(pluss::msharp::RangeView::Of<std::string_view> y_features,
                                     std::optional<std::string_view> x_feature) const;
  pluss::pl::Studio impl_box(pluss::msharp::RangeView::Of<std::string_view> features) const;
  pluss::pl::Studio impl_violin(pluss::msharp::RangeView::Of<std::string_view> features) const;
  pluss::pl::Studio impl_violin(std::string_view value_feature, std::string_view group_feature) const;
  pluss::pl::Studio impl_hist(std::string_view feature, std::int64_t bins) const;
  pluss::pl::Studio impl_hist(pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t bins) const;
  pluss::pl::Studio impl_density(pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t points) const;
  pluss::pl::Studio impl_pie(std::string_view value_feature,
                             std::optional<std::string_view> label_feature,
                             const PiePlotOptions& options) const;
  pluss::pl::Studio impl_heatmap(pluss::msharp::RangeView::Of<std::string_view> features) const;
  pluss::pl::Studio impl_heatmap(pluss::msharp::RangeView::Of<std::string_view> features,
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
  pluss::pl::Studio impl_corr_heatmap(pluss::msharp::RangeView::Of<std::string_view> features) const;
  pluss::pl::Studio impl_corr_heatmap(pluss::msharp::RangeView::Of<std::string_view> features,
                                      const CorrHeatmapPlotOptions& options) const;
  pluss::pl::Studio impl_scatter_matrix(pluss::msharp::RangeView::Of<std::string_view> features) const;
  pluss::pl::Studio impl_scatter_matrix(pluss::msharp::RangeView::Of<std::string_view> features,
                                        const ScatterMatrixPlotOptions& options) const;
  pluss::pl::Studio impl_parallel(
      pluss::msharp::RangeView::Of<std::string_view> features,
      std::optional<std::string_view> class_feature,
      const ParallelPlotOptions& options) const;
  pluss::pl::Studio impl_andrews_curves(
      pluss::msharp::RangeView::Of<std::string_view> features,
      std::optional<std::string_view> class_feature,
      const AndrewsCurvesPlotOptions& options) const;
  pluss::pl::Studio impl_radviz(pluss::msharp::RangeView::Of<std::string_view> features,
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
  pluss::pl::Studio impl_brief(pluss::msharp::RangeView::Of<std::string_view> features,
                               const BriefPlotOptions& options) const;
  pluss::pl::Studio impl_auto_plot(pluss::msharp::RangeView::Of<std::string_view> features) const;
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
