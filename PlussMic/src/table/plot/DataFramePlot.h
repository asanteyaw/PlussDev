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

// Which features a plot draws when they are not each named: all_features (every numeric feature),
// include_features (exactly these, in this order) or exclude_features (every numeric feature except
// these). The three are alternatives. With no names and none of the three set, the line-like plots
// (line, step, area, bar and their stacked forms) and the column-set plots (hist, box, ...) draw every
// numeric feature, exactly as if all_features were set; the selection itself is resolved by the
// shared flash rules, which also produce the error messages.
struct FeatureSelection {
  bool all_features{false};
  std::vector<std::string> include_features{};
  std::vector<std::string> exclude_features{};

  [[nodiscard]] bool any() const noexcept {
    return all_features || !include_features.empty() || !exclude_features.empty();
  }
};

namespace detail {
// Translates the PlussMic feature vocabulary to flash's column selection and resolves it there.
// With no knob set and `shortcut_all`, every numeric feature is selected (flash's all_columns).
[[nodiscard]] inline std::vector<std::string> resolve_feature_selection(
    const FeatureSelection& selection,
    const std::vector<std::string>& all_names,
    const std::vector<std::string>& numeric_names,
    std::string_view where,
    const std::optional<std::string>& x_name = std::nullopt,
    bool shortcut_all = true) {
  pluss::pl::flash::ColumnSelection flash_selection;
  flash_selection.all_columns = selection.all_features || (!selection.any() && shortcut_all);
  flash_selection.include_columns = selection.include_features;
  flash_selection.exclude_columns = selection.exclude_features;
  pluss::pl::flash::detail::ColumnWords words;
  words.noun = "feature";
  words.nouns = "features";
  words.all = "all_features";
  words.include = "include_features";
  words.exclude = "exclude_features";
  const auto picked = pluss::pl::flash::detail::resolve_column_selection(
      flash_selection, all_names, numeric_names, where, x_name, words);
  PLUSS_CHECK(picked.has_value(), std::string(where) + ": no features named; name them or set options.features");
  return *picked;
}
} // namespace detail

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
  // Accessor-level feature selection: applies to every plot made from this accessor that works on a set
  // of features. A plot's own options.features overrides it for that call, and features named in the call
  // override both. With none set, a call that names no features draws every numeric feature.
  bool all_features{false};
  std::vector<std::string> include_features{};
  std::vector<std::string> exclude_features{};

  [[nodiscard]] FeatureSelection selection() const {
    return FeatureSelection{all_features, include_features, exclude_features};
  }
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
  FeatureSelection features{};
};

struct HeatmapPlotOptions {
  bool annotate_levels{false};
  bool scale_guide{true};
  bool borderless{false};
  FeatureSelection features{};
};

struct CorrHeatmapPlotOptions {
  bool annotate_levels{true};
  bool scale_guide{true};
  bool borderless{false};
  FeatureSelection features{};
};

struct ScatterMatrixPlotOptions {
  bool corner{false};
  pluss::pl::flash::PairPlotOptions pair{};
  FeatureSelection features{};
};

struct ParallelPlotOptions {
  std::vector<std::string> labels{};
  bool normalize{true};
  std::int64_t max_points{1200};
  double alpha{0.24};
  float trace_width{0.9F};
  bool show_grid{true};
  bool show_legend{true};
  FeatureSelection features{};
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
  FeatureSelection features{};
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
  FeatureSelection features{};
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
  FeatureSelection features{};
};

struct LinePlotOptions {
  double alpha{1.0};
  double line_width{1.6};
  std::optional<pluss::pl::LimitMethod> y_limit_method{pluss::pl::LimitMethod::Padded};
  XYAlignOptions align{};
  FeatureSelection features{};
};

struct StepPlotOptions {
  double alpha{1.0};
  double line_width{1.5};
  XYAlignOptions align{};
  FeatureSelection features{};
};

struct AreaPlotOptions {
  double alpha{0.88};
  double line_width{1.3};
  XYAlignOptions align{};
  FeatureSelection features{};
};

struct AreaStackedPlotOptions {
  double alpha{0.7};
  double line_width{1.2};
  FeatureSelection features{};
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
  FeatureSelection features{};
};

struct BoxPlotOptions {
  double alpha{0.95};
  double line_width{1.1};
  FeatureSelection features{};
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
  FeatureSelection features{};
};

struct HistPlotOptions {
  std::int64_t bins{20};
  double alpha{0.95};
  double line_width{1.0};
  FeatureSelection features{};
};

struct DensityPlotOptions {
  std::int64_t points{128};
  double alpha{1.0};
  double line_width{1.5};
  FeatureSelection features{};
};

struct PiePlotOptions {
  bool show_label{true};
  bool show_percent{true};
  bool show_value{false};
  int precision{1};
  std::string separator{" | "};
};

struct AutoPlotOptions {
  FeatureSelection features{};
};

class ShelfPlotAccessor {
public:
  explicit ShelfPlotAccessor(Shelf value, DataFramePlotOptions options = {});

  pluss::pl::Studio line() const;
  pluss::pl::Studio line(const Shelf& x) const;
  pluss::pl::Studio line(const LinePlotOptions& options) const;
  pluss::pl::Studio line(const Shelf& x, const LinePlotOptions& options) const;

  pluss::pl::Studio step() const;
  pluss::pl::Studio step(const Shelf& x) const;
  pluss::pl::Studio step(const StepPlotOptions& options) const;
  pluss::pl::Studio step(const Shelf& x, const StepPlotOptions& options) const;

  pluss::pl::Studio area() const;
  pluss::pl::Studio area(const Shelf& x) const;
  pluss::pl::Studio area(const AreaPlotOptions& options) const;
  pluss::pl::Studio area(const Shelf& x, const AreaPlotOptions& options) const;

  pluss::pl::Studio scatter(const Shelf& x) const;
  pluss::pl::Studio scatter(const Shelf& x, const ScatterPlotOptions& options) const;
  pluss::pl::Studio scatter(const Shelf& x, const Shelf& hue) const;
  pluss::pl::Studio scatter(const Shelf& x, const Shelf& hue, const ScatterPlotOptions& options) const;

  pluss::pl::Studio bar() const;
  pluss::pl::Studio bar(const Shelf& x) const;
  pluss::pl::Studio bar(const BarPlotOptions& options) const;
  pluss::pl::Studio bar(const Shelf& x, const BarPlotOptions& options) const;

  pluss::pl::Studio box(const BoxPlotOptions& options = {}) const;
  pluss::pl::Studio violin(const ViolinPlotOptions& options = {}) const;
  pluss::pl::Studio violin(const Shelf& group) const;
  pluss::pl::Studio violin(const Shelf& group, const ViolinPlotOptions& options) const;

  pluss::pl::Studio hist(std::int64_t bins = 20) const;
  pluss::pl::Studio hist(const HistPlotOptions& options) const;

  pluss::pl::Studio density(std::int64_t points = 128) const;
  pluss::pl::Studio density(const DensityPlotOptions& options) const;

  pluss::pl::Studio pie() const;
  pluss::pl::Studio pie(const PiePlotOptions& options) const;
  pluss::pl::Studio pie(const Shelf& labels) const;
  pluss::pl::Studio pie(const Shelf& labels, const PiePlotOptions& options) const;

  pluss::pl::Studio lag(const LagPlotOptions& options = {}) const;
  pluss::pl::Studio act(const ActPlotOptions& options = {}) const;
  pluss::pl::Studio bootstrap(const BootstrapPlotOptions& options = {}) const;
  pluss::pl::Studio brief(const BriefPlotOptions& options = {}) const;
  pluss::pl::Studio& brief(pluss::pl::Studio& figure,
                           const BriefPlotOptions& options = {}) const;
  pluss::pl::Studio auto_plot() const;
  pluss::pl::Studio auto_plot(const AutoPlotOptions& options) const;

private:
  Shelf base_{};
  DataFramePlotOptions options_{};
};

class DataFramePlotAccessor {
public:
  explicit DataFramePlotAccessor(DataFrame frame, DataFramePlotOptions options = {});

  // x first, then y: line(x, y) or line(x, {y1, y2}); a single name is y against the case labels.
  // With no names, options.features (all_features, include_features or exclude_features) picks the y features,
  // drawn against x or the case labels. With neither, every numeric feature is drawn (all_features).
  pluss::pl::Studio line() const { return line_y(pluss::msharp::RangeView::Of<std::string_view>{}, std::nullopt, LinePlotOptions{}); }
  pluss::pl::Studio line(const LinePlotOptions& options) const { return line_y(pluss::msharp::RangeView::Of<std::string_view>{}, std::nullopt, options); }
  pluss::pl::Studio line(std::string_view x_feature, const LinePlotOptions& options) const { return line_y(pluss::msharp::RangeView::Of<std::string_view>{}, x_feature, options); }
  pluss::pl::Studio line(pluss::msharp::RangeView::Of<std::string_view> y_features, const LinePlotOptions& options) const { return line_y(y_features, std::nullopt, options); }
  pluss::pl::Studio line(std::initializer_list<std::string_view> y_features, const LinePlotOptions& options) const { return line_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), std::nullopt, options); }
  pluss::pl::Studio line(std::string_view y_feature) const { return line_y(y_feature, std::nullopt); }
  pluss::pl::Studio line(std::string_view x_feature, std::string_view y_feature) const { return line_y(y_feature, x_feature); }
  pluss::pl::Studio line(std::string_view x_feature, std::string_view y_feature, const LinePlotOptions& options) const { return line_y(y_feature, x_feature, options); }
  pluss::pl::Studio line(pluss::msharp::RangeView::Of<std::string_view> y_features) const { return line_y(y_features, std::nullopt, LinePlotOptions{}); }
  pluss::pl::Studio line(std::initializer_list<std::string_view> y_features) const { return line_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), std::nullopt, LinePlotOptions{}); }
  pluss::pl::Studio line(std::string_view x_feature, pluss::msharp::RangeView::Of<std::string_view> y_features) const { return line_y(y_features, x_feature, LinePlotOptions{}); }
  pluss::pl::Studio line(std::string_view x_feature, std::initializer_list<std::string_view> y_features) const { return line_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, LinePlotOptions{}); }
  pluss::pl::Studio line(std::string_view x_feature, pluss::msharp::RangeView::Of<std::string_view> y_features, const LinePlotOptions& options) const { return line_y(y_features, x_feature, options); }
  pluss::pl::Studio line(std::string_view x_feature, std::initializer_list<std::string_view> y_features, const LinePlotOptions& options) const { return line_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options); }
  // x first, then y: step(x, y) or step(x, {y1, y2}); a single name is y against the case labels.
  // With no names, options.features (all_features, include_features or exclude_features) picks the y features,
  // drawn against x or the case labels. With neither, every numeric feature is drawn (all_features).
  pluss::pl::Studio step() const { return step_y(pluss::msharp::RangeView::Of<std::string_view>{}, std::nullopt, StepPlotOptions{}); }
  pluss::pl::Studio step(const StepPlotOptions& options) const { return step_y(pluss::msharp::RangeView::Of<std::string_view>{}, std::nullopt, options); }
  pluss::pl::Studio step(std::string_view x_feature, const StepPlotOptions& options) const { return step_y(pluss::msharp::RangeView::Of<std::string_view>{}, x_feature, options); }
  pluss::pl::Studio step(pluss::msharp::RangeView::Of<std::string_view> y_features, const StepPlotOptions& options) const { return step_y(y_features, std::nullopt, options); }
  pluss::pl::Studio step(std::initializer_list<std::string_view> y_features, const StepPlotOptions& options) const { return step_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), std::nullopt, options); }
  pluss::pl::Studio step(std::string_view y_feature) const { return step_y(y_feature, std::nullopt); }
  pluss::pl::Studio step(std::string_view x_feature, std::string_view y_feature) const { return step_y(y_feature, x_feature); }
  pluss::pl::Studio step(std::string_view x_feature, std::string_view y_feature, const StepPlotOptions& options) const { return step_y(y_feature, x_feature, options); }
  pluss::pl::Studio step(pluss::msharp::RangeView::Of<std::string_view> y_features) const { return step_y(y_features, std::nullopt, StepPlotOptions{}); }
  pluss::pl::Studio step(std::initializer_list<std::string_view> y_features) const { return step_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), std::nullopt, StepPlotOptions{}); }
  pluss::pl::Studio step(std::string_view x_feature, pluss::msharp::RangeView::Of<std::string_view> y_features) const { return step_y(y_features, x_feature, StepPlotOptions{}); }
  pluss::pl::Studio step(std::string_view x_feature, std::initializer_list<std::string_view> y_features) const { return step_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, StepPlotOptions{}); }
  pluss::pl::Studio step(std::string_view x_feature, pluss::msharp::RangeView::Of<std::string_view> y_features, const StepPlotOptions& options) const { return step_y(y_features, x_feature, options); }
  pluss::pl::Studio step(std::string_view x_feature, std::initializer_list<std::string_view> y_features, const StepPlotOptions& options) const { return step_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options); }
  // x first, then y: area(x, y) or area(x, {y1, y2}); a single name is y against the case labels.
  // With no names, options.features (all_features, include_features or exclude_features) picks the y features,
  // drawn against x or the case labels. With neither, every numeric feature is drawn (all_features).
  pluss::pl::Studio area() const { return area_y(pluss::msharp::RangeView::Of<std::string_view>{}, std::nullopt, AreaPlotOptions{}); }
  pluss::pl::Studio area(const AreaPlotOptions& options) const { return area_y(pluss::msharp::RangeView::Of<std::string_view>{}, std::nullopt, options); }
  pluss::pl::Studio area(std::string_view x_feature, const AreaPlotOptions& options) const { return area_y(pluss::msharp::RangeView::Of<std::string_view>{}, x_feature, options); }
  pluss::pl::Studio area(pluss::msharp::RangeView::Of<std::string_view> y_features, const AreaPlotOptions& options) const { return area_y(y_features, std::nullopt, options); }
  pluss::pl::Studio area(std::initializer_list<std::string_view> y_features, const AreaPlotOptions& options) const { return area_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), std::nullopt, options); }
  pluss::pl::Studio area(std::string_view y_feature) const { return area_y(y_feature, std::nullopt); }
  pluss::pl::Studio area(std::string_view x_feature, std::string_view y_feature) const { return area_y(y_feature, x_feature); }
  pluss::pl::Studio area(std::string_view x_feature, std::string_view y_feature, const AreaPlotOptions& options) const { return area_y(y_feature, x_feature, options); }
  pluss::pl::Studio area(pluss::msharp::RangeView::Of<std::string_view> y_features) const { return area_y(y_features, std::nullopt, AreaPlotOptions{}); }
  pluss::pl::Studio area(std::initializer_list<std::string_view> y_features) const { return area_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), std::nullopt, AreaPlotOptions{}); }
  pluss::pl::Studio area(std::string_view x_feature, pluss::msharp::RangeView::Of<std::string_view> y_features) const { return area_y(y_features, x_feature, AreaPlotOptions{}); }
  pluss::pl::Studio area(std::string_view x_feature, std::initializer_list<std::string_view> y_features) const { return area_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, AreaPlotOptions{}); }
  pluss::pl::Studio area(std::string_view x_feature, pluss::msharp::RangeView::Of<std::string_view> y_features, const AreaPlotOptions& options) const { return area_y(y_features, x_feature, options); }
  pluss::pl::Studio area(std::string_view x_feature, std::initializer_list<std::string_view> y_features, const AreaPlotOptions& options) const { return area_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options); }
  // x first, then y: bar(x, y) or bar(x, {y1, y2}); a single name is y against the case labels.
  // With no names, options.features (all_features, include_features or exclude_features) picks the y features,
  // drawn against x or the case labels. With neither, every numeric feature is drawn (all_features).
  pluss::pl::Studio bar() const { return bar_y(pluss::msharp::RangeView::Of<std::string_view>{}, std::nullopt, BarPlotOptions{}); }
  pluss::pl::Studio bar(const BarPlotOptions& options) const { return bar_y(pluss::msharp::RangeView::Of<std::string_view>{}, std::nullopt, options); }
  pluss::pl::Studio bar(std::string_view x_feature, const BarPlotOptions& options) const { return bar_y(pluss::msharp::RangeView::Of<std::string_view>{}, x_feature, options); }
  pluss::pl::Studio bar(pluss::msharp::RangeView::Of<std::string_view> y_features, const BarPlotOptions& options) const { return bar_y(y_features, std::nullopt, options); }
  pluss::pl::Studio bar(std::initializer_list<std::string_view> y_features, const BarPlotOptions& options) const { return bar_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), std::nullopt, options); }
  pluss::pl::Studio bar(std::string_view y_feature) const { return bar_y(y_feature, std::nullopt); }
  pluss::pl::Studio bar(std::string_view x_feature, std::string_view y_feature) const { return bar_y(y_feature, x_feature); }
  pluss::pl::Studio bar(std::string_view x_feature, std::string_view y_feature, const BarPlotOptions& options) const { return bar_y(y_feature, x_feature, options); }
  pluss::pl::Studio bar(pluss::msharp::RangeView::Of<std::string_view> y_features) const { return bar_y(y_features, std::nullopt, BarPlotOptions{}); }
  pluss::pl::Studio bar(std::initializer_list<std::string_view> y_features) const { return bar_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), std::nullopt, BarPlotOptions{}); }
  pluss::pl::Studio bar(std::string_view x_feature, pluss::msharp::RangeView::Of<std::string_view> y_features) const { return bar_y(y_features, x_feature, BarPlotOptions{}); }
  pluss::pl::Studio bar(std::string_view x_feature, std::initializer_list<std::string_view> y_features) const { return bar_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, BarPlotOptions{}); }
  pluss::pl::Studio bar(std::string_view x_feature, pluss::msharp::RangeView::Of<std::string_view> y_features, const BarPlotOptions& options) const { return bar_y(y_features, x_feature, options); }
  pluss::pl::Studio bar(std::string_view x_feature, std::initializer_list<std::string_view> y_features, const BarPlotOptions& options) const { return bar_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options); }
  // x first, then y: area_stacked(x, y) or area_stacked(x, {y1, y2}); a single name is y against the case labels.
  // With no names, options.features (all_features, include_features or exclude_features) picks the y features,
  // drawn against x or the case labels. With neither, every numeric feature is drawn (all_features).
  pluss::pl::Studio area_stacked() const { return area_stacked_y(pluss::msharp::RangeView::Of<std::string_view>{}, std::nullopt, AreaStackedPlotOptions{}); }
  pluss::pl::Studio area_stacked(const AreaStackedPlotOptions& options) const { return area_stacked_y(pluss::msharp::RangeView::Of<std::string_view>{}, std::nullopt, options); }
  pluss::pl::Studio area_stacked(std::string_view x_feature, const AreaStackedPlotOptions& options) const { return area_stacked_y(pluss::msharp::RangeView::Of<std::string_view>{}, x_feature, options); }
  pluss::pl::Studio area_stacked(pluss::msharp::RangeView::Of<std::string_view> y_features, const AreaStackedPlotOptions& options) const { return area_stacked_y(y_features, std::nullopt, options); }
  pluss::pl::Studio area_stacked(std::initializer_list<std::string_view> y_features, const AreaStackedPlotOptions& options) const { return area_stacked_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), std::nullopt, options); }
  pluss::pl::Studio area_stacked(pluss::msharp::RangeView::Of<std::string_view> y_features) const { return area_stacked_y(y_features, std::nullopt, AreaStackedPlotOptions{}); }
  pluss::pl::Studio area_stacked(std::initializer_list<std::string_view> y_features) const { return area_stacked_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), std::nullopt, AreaStackedPlotOptions{}); }
  pluss::pl::Studio area_stacked(std::string_view x_feature, pluss::msharp::RangeView::Of<std::string_view> y_features) const { return area_stacked_y(y_features, x_feature, AreaStackedPlotOptions{}); }
  pluss::pl::Studio area_stacked(std::string_view x_feature, std::initializer_list<std::string_view> y_features) const { return area_stacked_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, AreaStackedPlotOptions{}); }
  pluss::pl::Studio area_stacked(std::string_view x_feature, pluss::msharp::RangeView::Of<std::string_view> y_features, const AreaStackedPlotOptions& options) const { return area_stacked_y(y_features, x_feature, options); }
  pluss::pl::Studio area_stacked(std::string_view x_feature, std::initializer_list<std::string_view> y_features, const AreaStackedPlotOptions& options) const { return area_stacked_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options); }
  // x first, then y: bar_stacked(x, y) or bar_stacked(x, {y1, y2}); a single name is y against the case labels.
  // With no names, options.features (all_features, include_features or exclude_features) picks the y features,
  // drawn against x or the case labels. With neither, every numeric feature is drawn (all_features).
  pluss::pl::Studio bar_stacked() const { return bar_stacked_y(pluss::msharp::RangeView::Of<std::string_view>{}, std::nullopt, BarStackedPlotOptions{}); }
  pluss::pl::Studio bar_stacked(const BarStackedPlotOptions& options) const { return bar_stacked_y(pluss::msharp::RangeView::Of<std::string_view>{}, std::nullopt, options); }
  pluss::pl::Studio bar_stacked(std::string_view x_feature, const BarStackedPlotOptions& options) const { return bar_stacked_y(pluss::msharp::RangeView::Of<std::string_view>{}, x_feature, options); }
  pluss::pl::Studio bar_stacked(pluss::msharp::RangeView::Of<std::string_view> y_features, const BarStackedPlotOptions& options) const { return bar_stacked_y(y_features, std::nullopt, options); }
  pluss::pl::Studio bar_stacked(std::initializer_list<std::string_view> y_features, const BarStackedPlotOptions& options) const { return bar_stacked_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), std::nullopt, options); }
  pluss::pl::Studio bar_stacked(pluss::msharp::RangeView::Of<std::string_view> y_features) const { return bar_stacked_y(y_features, std::nullopt, BarStackedPlotOptions{}); }
  pluss::pl::Studio bar_stacked(std::initializer_list<std::string_view> y_features) const { return bar_stacked_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), std::nullopt, BarStackedPlotOptions{}); }
  pluss::pl::Studio bar_stacked(std::string_view x_feature, pluss::msharp::RangeView::Of<std::string_view> y_features) const { return bar_stacked_y(y_features, x_feature, BarStackedPlotOptions{}); }
  pluss::pl::Studio bar_stacked(std::string_view x_feature, std::initializer_list<std::string_view> y_features) const { return bar_stacked_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, BarStackedPlotOptions{}); }
  pluss::pl::Studio bar_stacked(std::string_view x_feature, pluss::msharp::RangeView::Of<std::string_view> y_features, const BarStackedPlotOptions& options) const { return bar_stacked_y(y_features, x_feature, options); }
  pluss::pl::Studio bar_stacked(std::string_view x_feature, std::initializer_list<std::string_view> y_features, const BarStackedPlotOptions& options) const { return bar_stacked_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options); }
private:
  pluss::pl::Studio line_y(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio line_y(pluss::msharp::RangeView::Of<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio line_y(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const {
    return line_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature);
  }
  pluss::pl::Studio line_y(std::string_view y_feature,
                         std::optional<std::string_view> x_feature,
                         const LinePlotOptions& options) const;
  pluss::pl::Studio line_y(pluss::msharp::RangeView::Of<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const LinePlotOptions& options) const;
  pluss::pl::Studio line_y(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const LinePlotOptions& options) const {
    return line_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options);
  }
  pluss::pl::Studio step_y(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio step_y(pluss::msharp::RangeView::Of<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio step_y(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const {
    return step_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature);
  }
  pluss::pl::Studio step_y(std::string_view y_feature,
                         std::optional<std::string_view> x_feature,
                         const StepPlotOptions& options) const;
  pluss::pl::Studio step_y(pluss::msharp::RangeView::Of<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const StepPlotOptions& options) const;
  pluss::pl::Studio step_y(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const StepPlotOptions& options) const {
    return step_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options);
  }
  pluss::pl::Studio area_y(std::string_view y_feature,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area_y(pluss::msharp::RangeView::Of<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area_y(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature = std::nullopt) const {
    return area_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature);
  }
  pluss::pl::Studio area_y(std::string_view y_feature,
                         std::optional<std::string_view> x_feature,
                         const AreaPlotOptions& options) const;
  pluss::pl::Studio area_y(pluss::msharp::RangeView::Of<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const AreaPlotOptions& options) const;
  pluss::pl::Studio area_y(std::initializer_list<std::string_view> y_features,
                         std::optional<std::string_view> x_feature,
                         const AreaPlotOptions& options) const {
    return area_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options);
  }
  pluss::pl::Studio area_stacked_y(pluss::msharp::RangeView::Of<std::string_view> y_features,
                                 std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio area_stacked_y(std::initializer_list<std::string_view> y_features,
                                 std::optional<std::string_view> x_feature = std::nullopt) const {
    return area_stacked_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature);
  }
  pluss::pl::Studio area_stacked_y(pluss::msharp::RangeView::Of<std::string_view> y_features,
                                 std::optional<std::string_view> x_feature,
                                 const AreaStackedPlotOptions& options) const;
  pluss::pl::Studio area_stacked_y(std::initializer_list<std::string_view> y_features,
                                 std::optional<std::string_view> x_feature,
                                 const AreaStackedPlotOptions& options) const {
    return area_stacked_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options);
  }
public:
  pluss::pl::Studio scatter(std::string_view x_feature,
                            std::string_view y_feature,
                            std::optional<std::string_view> hue_feature = std::nullopt) const;
  pluss::pl::Studio scatter(std::string_view x_feature,
                            std::string_view y_feature,
                            std::optional<std::string_view> hue_feature,
                            const ScatterPlotOptions& options) const;
private:
  pluss::pl::Studio bar_y(std::string_view y_feature,
                        std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio bar_y(std::string_view y_feature,
                        std::optional<std::string_view> x_feature,
                        const BarPlotOptions& options) const;
  pluss::pl::Studio bar_y(pluss::msharp::RangeView::Of<std::string_view> y_features,
                        std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio bar_y(std::initializer_list<std::string_view> y_features,
                        std::optional<std::string_view> x_feature = std::nullopt) const {
    return bar_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature);
  }
  pluss::pl::Studio bar_y(pluss::msharp::RangeView::Of<std::string_view> y_features,
                        std::optional<std::string_view> x_feature,
                        const BarPlotOptions& options) const;
  pluss::pl::Studio bar_y(std::initializer_list<std::string_view> y_features,
                        std::optional<std::string_view> x_feature,
                        const BarPlotOptions& options) const {
    return bar_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options);
  }
  pluss::pl::Studio bar_stacked_y(pluss::msharp::RangeView::Of<std::string_view> y_features,
                                std::optional<std::string_view> x_feature = std::nullopt) const;
  pluss::pl::Studio bar_stacked_y(std::initializer_list<std::string_view> y_features,
                                std::optional<std::string_view> x_feature = std::nullopt) const {
    return bar_stacked_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature);
  }
  pluss::pl::Studio bar_stacked_y(pluss::msharp::RangeView::Of<std::string_view> y_features,
                                std::optional<std::string_view> x_feature,
                                const BarStackedPlotOptions& options) const;
  pluss::pl::Studio bar_stacked_y(std::initializer_list<std::string_view> y_features,
                                std::optional<std::string_view> x_feature,
                                const BarStackedPlotOptions& options) const {
    return bar_stacked_y(pluss::msharp::RangeView::Of<std::string_view>(y_features), x_feature, options);
  }
public:
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
  // With no names: the accessor's or options.features selection, else every numeric feature, overlaid on one axes.
  pluss::pl::Studio density() const { return density(pluss::msharp::RangeView::Of<std::string_view>{}, DensityPlotOptions{}); }
  pluss::pl::Studio density(const DensityPlotOptions& options) const { return density(pluss::msharp::RangeView::Of<std::string_view>{}, options); }
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
  friend class ShelfPlotAccessor;

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
  pluss::pl::Studio impl_auto_plot(pluss::msharp::RangeView::Of<std::string_view> features,
                                   const FeatureSelection& selection) const;
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
