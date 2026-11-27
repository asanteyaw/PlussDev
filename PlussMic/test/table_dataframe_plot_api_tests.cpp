#include <functional>
#include <algorithm>
#include <cmath>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::pl::TraceKind;
using pluss::table::DataFrame;
using pluss::table::Shelf;

[[noreturn]] void fail(std::string_view message) {
  throw std::runtime_error(std::string(message));
}

void expect(bool condition, std::string_view message) {
  if (!condition) {
    fail(message);
  }
}

template <class Fn>
void expect_throw(Fn&& fn, std::string_view message) {
  bool threw = false;
  try {
    fn();
  } catch (...) {
    threw = true;
  }
  if (!threw) {
    fail(message);
  }
}

DataFrame make_xy_frame() {
  return DataFrame({
      {"x", Shelf(pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}))},
      {"y", Shelf(pluss::make_series(std::vector<double>{1.0, 1.4, 2.2, 3.0}))},
      {"z", Shelf(pluss::make_series(std::vector<double>{0.8, 1.0, 1.6, 2.4}))},
      {"grp", Shelf(pluss::make_note({"alpha", "alpha", "beta", "beta"}))},
  });
}

DataFrame make_wide_numeric_frame(std::int64_t nfeatures) {
  std::vector<std::pair<std::string, Shelf>> columns;
  columns.reserve(static_cast<std::size_t>(nfeatures));
  for (const auto idx : std::views::iota(std::int64_t{0}, nfeatures)) {
    columns.emplace_back(
        "f" + std::to_string(idx + 1),
        Shelf(pluss::make_series(std::vector<double>{
            static_cast<double>(idx),
            static_cast<double>(idx + 1),
            static_cast<double>(idx + 2),
            static_cast<double>(idx + 3),
        })));
  }
  return DataFrame(std::move(columns));
}

void test_shelf_plot_apis() {
  const auto x = Shelf(pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0})).name("x");
  const auto y = Shelf(pluss::make_series(std::vector<double>{1.0, 1.4, 2.2, 3.0}))
                     .label().set(Shelf(pluss::make_note({"r0", "r1", "r2", "r3"})))
                     .name("y");
  const auto grp = Shelf(pluss::make_note({"alpha", "alpha", "beta", "beta"})).name("grp");

  const auto fig_line = y.plot().line(x);
  expect(fig_line.panes().front().trace_count() == 1, "shelf.plot().line(x) should emit one trace");
  expect(fig_line.panes().front().traces().front().kind() == TraceKind::Trend,
         "shelf.plot().line(x) should emit trend trace");

  const auto fig_bar = y.plot().bar();
  expect(fig_bar.panes().front().trace_count() == 1, "shelf.plot().bar should emit one trace");
  expect(fig_bar.panes().front().traces().front().kind() == TraceKind::Columns,
         "shelf.plot().bar should emit columns trace");

  const auto fig_hist = y.plot().hist(7);
  expect(fig_hist.panes().front().trace_count() == 1, "shelf.plot().hist should emit one trace");
  expect(fig_hist.panes().front().traces().front().kind() == TraceKind::Bucket,
         "shelf.plot().hist should emit bucket trace");

  const auto fig_density = y.plot().density(64);
  expect(fig_density.panes().front().trace_count() == 1, "shelf.plot().density should emit one trace");
  expect(fig_density.panes().front().traces().front().kind() == TraceKind::Trend,
         "shelf.plot().density should emit trend trace");

  const auto fig_pie = y.plot().pie(grp);
  expect(fig_pie.panes().front().trace_count() == 1, "shelf.plot().pie(labels) should emit one trace");
  expect(fig_pie.panes().front().traces().front().kind() == TraceKind::Wedge,
         "shelf.plot().pie(labels) should emit wedge trace");

  const auto fig_scatter = y.plot().scatter(x, grp);
  expect(fig_scatter.panes().front().trace_count() == 2, "shelf.plot().scatter(x, hue) should split by hue");
  expect(fig_scatter.panes().front().traces().front().kind() == TraceKind::Cloud,
         "shelf.plot().scatter(x, hue) should emit cloud trace");

  const auto fig_lag = y.plot().lag();
  expect(fig_lag.panes().front().trace_count() == 1, "shelf.plot().lag should emit one trace");
  expect(fig_lag.panes().front().traces().front().kind() == TraceKind::Cloud,
         "shelf.plot().lag should emit cloud trace");

  const auto fig_act = y.plot().act();
  expect(fig_act.panes().front().trace_count() >= 1, "shelf.plot().act should emit at least one trace");
  expect(fig_act.panes().front().traces().front().kind() == TraceKind::Spike,
         "shelf.plot().act should emit spike trace first");

  const auto fig_bootstrap = y.plot().bootstrap();
  expect(fig_bootstrap.panes().front().trace_count() == 1,
         "shelf.plot().bootstrap should emit one trace");
  expect(fig_bootstrap.panes().front().traces().front().kind() == TraceKind::Bucket,
         "shelf.plot().bootstrap should emit bucket trace");

  const auto fig_auto = y.plot().auto_plot();
  expect(fig_auto.panes().front().trace_count() == 1, "shelf.plot().auto_plot should emit one trace");
  expect(fig_auto.panes().front().traces().front().kind() == TraceKind::Columns,
         "shelf.plot().auto_plot should prefer columns for short labeled threads");
}

void test_object_plot_apis() {
  const auto df = make_xy_frame();

  const auto fig_hist_obj = df.plot().hist("y", 7);
  expect(fig_hist_obj.pane_count() == 1, "plot().hist should produce one pane");
  expect(fig_hist_obj.panes().front().trace_count() == 1, "plot().hist should emit one trace");
  expect(fig_hist_obj.panes().front().traces().front().kind() == TraceKind::Bucket,
         "plot().hist should emit bucket trace");

  const auto fig_hist_multi = df.plot().hist({"x", "y", "z"}, 7);
  expect(fig_hist_multi.pane_count() == 3, "plot().hist(features) should produce one pane per feature");

  const auto fig_hist_all = df.plot().hist(7);
  expect(fig_hist_all.pane_count() == 3, "plot().hist(bins) should default to all numeric features");

  const auto fig_line = df.plot().line("x", {"y", "z"});
  expect(fig_line.panes().front().trace_count() == 2, "plot().line should emit one trace per y feature");
  expect(fig_line.panes().front().traces().front().kind() == TraceKind::Trend,
         "plot().line should emit trend traces");

  const auto fig_violin = df.plot().violin({"y", "z"});
  expect(fig_violin.panes().front().trace_count() == 1, "plot().violin(features) should emit one trace");
  expect(fig_violin.panes().front().traces().front().kind() == TraceKind::Violin,
         "plot().violin(features) should emit violin trace");

  const auto fig_box = df.plot().box({"y", "z"});
  expect(fig_box.panes().front().trace_count() == 1, "plot().box(features) should emit one trace");
  expect(fig_box.panes().front().traces().front().kind() == TraceKind::Quartet,
         "plot().box(features) should emit quartet trace");

  const auto fig_violin_grouped = df.plot().violin("y", "grp");
  expect(fig_violin_grouped.panes().front().trace_count() == 1, "plot().violin(value, group) should emit one trace");
  expect(fig_violin_grouped.panes().front().traces().front().kind() == TraceKind::Violin,
         "plot().violin(value, group) should emit violin trace");

  const auto fig_density_multi = df.plot().density({"y", "z"}, 64);
  expect(fig_density_multi.panes().front().trace_count() == 2,
         "plot().density(features) should emit one trace per feature");
  expect(fig_density_multi.panes().front().traces().front().kind() == TraceKind::Trend,
         "plot().density(features) should emit trend traces");

  const auto fig_density = df.plot().density("y", 64);
  expect(fig_density.panes().front().trace_count() == 1, "plot().density(feature) should emit one trace");
  expect(fig_density.panes().front().traces().front().kind() == TraceKind::Trend,
         "plot().density(feature) should emit trend trace");

  const auto fig_pie = df.plot().pie("y", "grp");
  expect(fig_pie.panes().front().trace_count() == 1, "plot().pie should emit one trace");
  expect(fig_pie.panes().front().traces().front().kind() == TraceKind::Wedge,
         "plot().pie should emit wedge trace");

  const auto fig_parallel = df.plot().parallel({"x", "y", "z"}, "grp");
  expect(fig_parallel.panes().front().trace_count() > 0,
         "plot().parallel should emit one trend trace per sampled case");
  expect(fig_parallel.panes().front().traces().front().kind() == TraceKind::Trend,
         "plot().parallel should emit trend traces");

  const auto fig_andrews = df.plot().andrews_curves({"x", "y", "z"}, "grp");
  expect(fig_andrews.panes().front().trace_count() > 0,
         "plot().andrews_curves should emit one trend trace per sampled case");
  expect(fig_andrews.panes().front().traces().front().kind() == TraceKind::Trend,
         "plot().andrews_curves should emit trend traces");

  const auto fig_radviz = df.plot().radviz({"x", "y", "z"}, "grp");
  expect(fig_radviz.panes().front().trace_count() > 0, "plot().radviz should emit cloud traces");
  expect(fig_radviz.panes().front().traces().front().kind() == TraceKind::Cloud,
         "plot().radviz should emit cloud traces");

  const auto fig_lag = df.plot().lag("x");
  expect(fig_lag.panes().front().trace_count() == 1, "plot().lag should emit one cloud trace");
  expect(fig_lag.panes().front().traces().front().kind() == TraceKind::Cloud,
         "plot().lag should emit cloud trace");

  const auto fig_acf = df.plot().act("x");
  expect(fig_acf.panes().front().trace_count() >= 1,
         "plot().act should emit at least one spike trace");
  expect(fig_acf.panes().front().traces().front().kind() == TraceKind::Spike,
         "plot().act should emit spike trace first");

  const auto fig_bootstrap = df.plot().bootstrap("x");
  expect(fig_bootstrap.panes().front().trace_count() == 1,
         "plot().bootstrap should emit one histogram trace");
  expect(fig_bootstrap.panes().front().traces().front().kind() == TraceKind::Bucket,
         "plot().bootstrap should emit bucket trace");

  const auto fig_brief = df.plot().brief({"x", "y", "z"});
  expect(fig_brief.panes().front().caption_text().size() > 0, "plot().brief should populate caption text");
}

void test_scatter_heatmap_and_validation() {
  const auto df = make_xy_frame();
  const auto fig_scatter = df.plot().scatter("x", "y", "grp");
  expect(fig_scatter.panes().front().trace_count() == 2, "plot().scatter(hue) should split by hue");
  expect(fig_scatter.panes().front().traces().front().kind() == TraceKind::Cloud,
         "plot().scatter should emit cloud trace");

  pluss::table::ScatterPlotOptions no_fit_options;
  const auto fig_scatter_no_fit = df.plot().scatter("x", "y", std::nullopt, no_fit_options);
  expect(fig_scatter_no_fit.panes().front().trace_count() == 1,
         "plot().scatter with fit_line=false (default) should emit only the cloud trace");

  pluss::table::ScatterPlotOptions fit_options;
  fit_options.fit_line = true;
  const auto fig_scatter_fit = df.plot().scatter("x", "y", std::nullopt, fit_options);
  expect(fig_scatter_fit.panes().front().trace_count() == 2,
         "plot().scatter with fit_line=true should add a fit trace");
  expect(fig_scatter_fit.panes().front().traces().back().kind() == TraceKind::Trend,
         "plot().scatter with fit_line=true should emit a trend trace for the fit line");

  const auto fig_scatter_fit_hue = df.plot().scatter("x", "y", "grp", fit_options);
  expect(fig_scatter_fit_hue.panes().front().trace_count() == 4,
         "plot().scatter with fit_line=true and hue should add one fit trace per hue group");

  expect_throw(
      [&]() {
        pluss::table::ScatterPlotOptions bad_fit_options;
        bad_fit_options.fit_line = true;
        const auto single_row = DataFrame({
            {"x", Shelf(pluss::make_series(std::vector<double>{0.0}))},
            {"y", Shelf(pluss::make_series(std::vector<double>{1.0}))},
        });
        (void)single_row.plot().scatter("x", "y", std::nullopt, bad_fit_options);
      },
      "plot().scatter with fit_line=true and fewer than two points should throw");

  const auto surface = DataFrame({
      {"maturity", Shelf(pluss::make_note({"1M", "1M", "3M", "3M"}))},
      {"delta_bucket", Shelf(pluss::make_note({"25P", "ATM", "25P", "ATM"}))},
      {"implied_vol", Shelf(pluss::make_series(std::vector<double>{0.30, 0.25, 0.33, 0.28}))},
  });
  const auto fig_heatmap = surface.plot().heatmap("maturity", "delta_bucket", "implied_vol");
  expect(fig_heatmap.panes().front().trace_count() == 1, "plot().heatmap(pivot) should emit one trace");
  expect(fig_heatmap.panes().front().traces().front().kind() == TraceKind::Fieldmap,
         "plot().heatmap(pivot) should emit fieldmap trace");
  expect(fig_heatmap.panes().front().scale_guide_enabled(),
         "plot().heatmap(pivot) should enable scale guide by default");

  pluss::table::HeatmapPlotOptions annotated_options;
  annotated_options.annotate_levels = true;
  annotated_options.scale_guide = false;
  const auto annotated_heatmap = surface.plot().heatmap(
      "maturity",
      "delta_bucket",
      "implied_vol",
      DataFrame::GroupOp::Mean,
      annotated_options);
  const auto annotated_svg = annotated_heatmap.to_svg();
  expect(annotated_svg.find(">0.3</text>") != std::string::npos,
         "plot().heatmap(annotate_levels) should render cell values");

  const auto fig_corr = df.plot().corr_heatmap({"x", "y", "z"});
  expect(fig_corr.panes().front().traces().front().annotate_levels(),
         "plot().corr_heatmap should annotate cell values by default");

  expect_throw([&]() { (void)df.plot().scatter("x", "missing_feature"); },
               "plot().scatter with missing feature should throw");
}

void test_scatter_matrix_guardrails() {
  const auto wide = make_wide_numeric_frame(15);
  const auto fig_auto = wide.plot().scatter_matrix();
  expect(fig_auto.pane_count() == 225,
         "plot().scatter_matrix() should keep the full numeric feature set when features are implicit");
  expect(fig_auto.to_svg().find("<svg") != std::string::npos,
         "plot().scatter_matrix() should render wide numeric frames without truncating features");

  std::vector<std::string_view> explicit_features;
  explicit_features.reserve(13);
  for (const auto idx : std::views::iota(std::int64_t{0}, std::int64_t{13})) {
    explicit_features.push_back(wide.features()[static_cast<std::size_t>(idx)]);
  }
  const auto fig_explicit =
      wide.plot().scatter_matrix(pluss::msharp::RangeView::Of<std::string_view>(explicit_features));
  expect(fig_explicit.pane_count() == 169,
         "plot().scatter_matrix(features) should honor explicit feature selection");
}

void test_plot_options_defaults() {
  const auto df = make_xy_frame();
  pluss::table::DataFramePlotOptions options;
  options.default_hue_feature = "grp";
  options.default_bins = 9;
  options.width = 920.0;
  options.height = 640.0;
  options.use_tight_layout = true;
  options.x_tick_rotation_deg = 35.0;
  options.y_tick_rotation_deg = 90.0;
  options.x_label_rotation_deg = 0.0;
  options.y_label_rotation_deg = -90.0;
  options.x_tick_format = pluss::pl::TickFormat::Plain;
  options.y_tick_format = pluss::pl::TickFormat::Plain;
  options.y_label_gap_points = 24.0;

  const auto fig_scatter = df.plot(options).scatter("x", "y");
  expect(fig_scatter.panes().front().trace_count() == 2, "plot(options).scatter should use default hue feature");
  expect(fig_scatter.extent().width == 920.0 && fig_scatter.extent().height == 640.0,
         "plot(options) should apply figure size defaults");
  expect(fig_scatter.panes().front().x_axis().label_rotation_deg.has_value(),
         "plot(options).scatter should propagate x label rotation");
  expect(fig_scatter.panes().front().y_axis().label_rotation_deg.has_value(),
         "plot(options).scatter should propagate y label rotation");
  expect(std::abs(*fig_scatter.panes().front().y_axis().label_rotation_deg + 90.0) < 1e-12,
         "plot(options).scatter should use requested y label rotation");

  const auto fig_hist = df.plot(options).hist("y");
  expect(fig_hist.panes().front().trace_count() == 1, "plot(options).hist should still emit one trace");
  expect(fig_hist.panes().front().traces().front().kind() == TraceKind::Bucket,
         "plot(options).hist should emit bucket trace");

  pluss::table::BarPlotOptions bar_options;
  bar_options.ci_enabled = true;
  bar_options.ci_level = 0.95;
  const auto fig_bar_ci = df.plot(options).bar("grp", "y", bar_options);
  expect(fig_bar_ci.panes().front().trace_count() == 2, "plot(options).bar with CI should emit bar + whisker");
  const auto& traces = fig_bar_ci.panes().front().traces();
  const bool has_whisk = std::any_of(traces.begin(), traces.end(), [](const auto& trace) {
    return trace.kind() == TraceKind::Whisk;
  });
  expect(has_whisk, "plot(options).bar with CI should include whisk trace");

  pluss::table::BarPlotOptions horizontal_bar_options;
  horizontal_bar_options.horizontal = true;
  const auto fig_bar_horizontal = df.plot(options).bar("grp", "y", horizontal_bar_options);
  expect(fig_bar_horizontal.panes().front().trace_count() == 1,
         "plot(options).bar(horizontal) should emit one columns trace without CI");
  expect(fig_bar_horizontal.panes().front().traces().front().kind() == TraceKind::Columns,
         "plot(options).bar(horizontal) should still emit columns trace kind");
  expect(fig_bar_horizontal.panes().front().y_axis().ticks.has_value(),
         "plot(options).bar(horizontal) should place categorical ticks on y-axis");
  expect(fig_bar_horizontal.panes().front().x_axis().label == "y",
         "plot(options).bar(horizontal) should label value axis on x-axis");

  pluss::table::ScatterMatrixPlotOptions scatter_matrix_options;
  scatter_matrix_options.pair.triangle = pluss::table::ScatterMatrixTriangle::Lower;
  scatter_matrix_options.pair.auto_tick_decimals = 1;
  const auto fig_corner = df.plot(options).scatter_matrix({"x", "y", "z"}, scatter_matrix_options);
  std::size_t trace_count = 0;
  for (const auto& pane : fig_corner.panes()) {
    trace_count += pane.trace_count();
  }
  expect(trace_count == 6, "plot(options).scatter_matrix(lower) should render only lower triangle + diagonal");
  expect(fig_corner.panes().front().x_axis().auto_tick_decimals.has_value(),
         "plot(options).scatter_matrix should propagate auto tick decimals");
  expect(*fig_corner.panes().front().x_axis().auto_tick_decimals == 1,
         "plot(options).scatter_matrix should use requested auto tick decimals");
  expect(fig_corner.panes().front().x_axis().tick_rotation_deg.has_value(),
         "plot(options).scatter_matrix should propagate x tick rotation");
  expect(fig_corner.panes().front().y_axis().tick_rotation_deg.has_value(),
         "plot(options).scatter_matrix should propagate y tick rotation");
  expect(std::abs(*fig_corner.panes().front().x_axis().tick_rotation_deg - 35.0) < 1e-12,
         "plot(options).scatter_matrix should use requested x tick rotation");
  expect(std::abs(*fig_corner.panes().front().y_axis().tick_rotation_deg - 90.0) < 1e-12,
         "plot(options).scatter_matrix should use requested y tick rotation");
  expect(fig_corner.panes().front().x_axis().label_rotation_deg.has_value(),
         "plot(options).scatter_matrix should propagate x label rotation");
  expect(fig_corner.panes().front().y_axis().label_rotation_deg.has_value(),
         "plot(options).scatter_matrix should propagate y label rotation");
  expect(std::abs(*fig_corner.panes().front().y_axis().label_rotation_deg + 90.0) < 1e-12,
         "plot(options).scatter_matrix should use requested y label rotation");
  expect(fig_corner.panes().front().y_axis().label_centered_on_axis,
         "plot(options).scatter_matrix should center left-edge y labels to avoid tick overlap");
  expect(fig_corner.panes().front().x_axis().tick_format == pluss::pl::TickFormat::Plain,
         "plot(options).scatter_matrix should propagate x tick format");
  expect(fig_corner.panes().front().y_axis().tick_format == pluss::pl::TickFormat::Plain,
         "plot(options).scatter_matrix should propagate y tick format");
  expect(fig_corner.panes().front().y_axis().label_gap.has_value(),
         "plot(options).scatter_matrix should propagate y label gap override");
  expect(std::abs(*fig_corner.panes().front().y_axis().label_gap - 24.0) < 1e-12,
         "plot(options).scatter_matrix should use requested y label gap override");

  pluss::table::HeatmapPlotOptions heatmap_options;
  heatmap_options.annotate_levels = true;
  const auto fig_heatmap = df.plot(options).heatmap({"x", "y"}, heatmap_options);
  expect(fig_heatmap.panes().front().traces().front().annotate_levels(),
         "plot(options).heatmap should honor heatmap_annotate_levels");
}

void test_corr_methods_and_plot3d() {
  const auto df = make_xy_frame();
  const auto pearson = df.features({"x", "y", "z"}).corr(false);
  const auto spearman = df.features({"x", "y", "z"}).corr(DataFrame::CorrMethod::Spearman, false);
  const auto kendall = df.features({"x", "y", "z"}).corr(DataFrame::CorrMethod::KendallTau, false);
  expect(pearson.ncases() == 3 && pearson.nfeatures() == 3, "corr(pearson) should return square matrix");
  expect(spearman.ncases() == 3 && spearman.nfeatures() == 3, "corr(spearman) should return square matrix");
  expect(kendall.ncases() == 3 && kendall.nfeatures() == 3, "corr(kendall) should return square matrix");

  const auto fig_scatter3 = df.plot3d().scatter("x", "y", "z");
  expect(fig_scatter3.panes().front().traces().front().kind() == TraceKind::Cloud3,
         "plot3d().scatter should emit cloud3 trace");

  const auto fig_line3 = df.plot3d().line("x", "y", "z");
  expect(fig_line3.panes().front().traces().front().kind() == TraceKind::Path3,
         "plot3d().line should emit path3 trace");

  const auto grid = DataFrame({
      {"gx", Shelf(pluss::make_series(std::vector<double>{0.0, 1.0, 0.0, 1.0}))},
      {"gy", Shelf(pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}))},
      {"gz", Shelf(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}))},
  });
  const auto fig_surface = grid.plot3d().surface("gx", "gy", "gz");
  expect(fig_surface.panes().front().traces().front().kind() == TraceKind::Sheet,
         "plot3d().surface should emit sheet trace");

  const auto fig_wire = grid.plot3d().wireframe("gx", "gy", "gz");
  expect(fig_wire.panes().front().traces().front().kind() == TraceKind::Lattice,
         "plot3d().wireframe should emit lattice trace");
}

std::string error_text(const std::function<void()>& body) {
  try {
    body();
  } catch (const pluss::Error& e) {
    return e.message();
  }
  return {};
}

bool has(const std::string& text, std::string_view needle) { return text.find(needle) != std::string::npos; }

void test_default_columns_and_date_axis() {
  const auto three = DataFrame({
      {"a", Shelf(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))},
      {"b", Shelf(pluss::make_series(std::vector<double>{4.0, 5.0, 7.0}))},
      {"c", Shelf(pluss::make_series(std::vector<double>{2.0, 1.0, 0.0}))},
  });
  // x first, then y.
  expect(three.plot().line("a", "b").panes().front().trace_count() == 1, "line(x, y) should draw one trace");
  expect(three.plot().line("b", "a").to_svg() != three.plot().line("a", "b").to_svg(), "line(x, y) should put the first name on x");
  expect(three.plot().line("a", {"b", "c"}).panes().front().trace_count() == 2, "line(x, {y1, y2}) should draw two traces");

  // The PlussMic shortcut: no names and no selection draws every numeric feature on one axes.
  const auto all = three.plot().line();
  expect(all.panes().size() == 1 && all.panes().front().trace_count() == 3,
         "plot().line() should draw every numeric feature on one axes");
  expect(three.plot().step().panes().front().trace_count() == 3 && three.plot().area().panes().front().trace_count() == 3 &&
             three.plot().bar().panes().front().trace_count() == 3,
         "step, area and bar share the shortcut");
  // The knobs, spoken in features.
  expect(three.plot().line({.features = {.all_features = true}}).panes().front().trace_count() == 3, "all_features");
  expect(three.plot().line({.features = {.include_features = {"c", "a"}}}).panes().front().trace_count() == 2, "include_features");
  expect(three.plot().line({.features = {.exclude_features = {"b"}}}).panes().front().trace_count() == 2, "exclude_features");
  expect(three.plot().line("a", {.features = {.all_features = true}}).panes().front().trace_count() == 2,
         "all_features with an explicit x leaves x out of the traces");
  expect(three.plot().bar({.features = {.exclude_features = {"a"}}}).panes().front().trace_count() == 2, "bar knobs");
  expect(three.plot().area_stacked({.features = {.all_features = true}}).panes().front().trace_count() == 3, "area_stacked knobs");
  expect(three.plot().bar_stacked({.features = {.include_features = {"a", "b"}}}).panes().front().trace_count() == 2, "bar_stacked knobs");

  // Misuse: the message says what was wrong and what to do, in PlussMic's words.
  const auto both = error_text([&] { (void)three.plot().line({.features = {.all_features = true, .exclude_features = {"a"}}}); });
  expect(has(both, "all_features and exclude_features cannot be combined") && has(both, "already means every numeric feature") &&
             has(both, "set exclude_features instead") && !has(both, "column"),
         "all_features + exclude_features: " + both);
  const auto all_inc = error_text([&] { (void)three.plot().line({.features = {.all_features = true, .include_features = {"a"}}}); });
  expect(has(all_inc, "all_features and include_features cannot be combined") && has(all_inc, "set include_features instead"),
         "all_features + include_features: " + all_inc);
  const auto inc_exc = error_text([&] {
    (void)three.plot().line({.features = {.include_features = {"a"}, .exclude_features = {"b"}}});
  });
  expect(has(inc_exc, "include_features and exclude_features cannot both be set") && has(inc_exc, "but not both"),
         "include + exclude: " + inc_exc);
  const auto unknown = error_text([&] { (void)three.plot().line({.features = {.include_features = {"aa"}}}); });
  expect(has(unknown, "unknown feature 'aa'") && has(unknown, "Did you mean 'a'?") && has(unknown, "Available features = [a, b, c]"),
         "unknown feature: " + unknown);
  const auto x_in = error_text([&] { (void)three.plot().line("a", {.features = {.include_features = {"a", "b"}}}); });
  expect(has(x_in, "also the x feature") && has(x_in, "Remove 'a' from include_features"), "x among the selection: " + x_in);

  const auto dated = DataFrame({
      {"date", Shelf(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z", "2024-03-01T00:00:00Z"}))},
      {"amount", Shelf(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))},
  });
  // The x axis is never guessed: the time column is x only when named.
  expect(dated.plot().line("date", "amount").to_svg().find("2024-02-01") != std::string::npos,
         "a named time column should label the x axis with dates");
  expect(dated.plot().line().panes().front().trace_count() == 1, "the shortcut draws the numeric feature only");
  expect(dated.plot().line("amount", "date").to_svg().find("2024-02-01") != std::string::npos,
         "a time column on the y axis should show dates");

  const auto text_only = DataFrame({{"s", Shelf(pluss::make_note(std::vector<std::string>{"a", "b"}))}});
  const auto none = error_text([&] { (void)text_only.plot().line(); });
  expect(has(none, "no numeric feature"), "plot().line() with no numeric feature should explain: " + none);
}

void test_feature_selection_precedence() {
  const auto three = DataFrame({
      {"a", Shelf(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))},
      {"b", Shelf(pluss::make_series(std::vector<double>{4.0, 5.0, 7.0}))},
      {"c", Shelf(pluss::make_series(std::vector<double>{2.0, 1.0, 0.0}))},
  });
  const auto traces = [](const pluss::pl::Studio& fig) { return fig.panes().front().trace_count(); };
  // No names and no selection: every numeric feature.
  expect(traces(three.plot().line()) == 3, "shortcut draws every numeric feature");
  // Accessor level: applies to every plot that works on a set of features.
  const auto acc = three.plot({.include_features = {"a", "b"}});
  expect(traces(acc.line()) == 2 && traces(acc.step()) == 2 && traces(acc.area()) == 2 && traces(acc.bar()) == 2,
         "accessor-level include_features should apply to line, step, area and bar");
  expect(acc.hist().panes().size() == 2, "accessor-level selection should apply to hist (one pane per feature)");
  expect(traces(acc.density()) == 2, "accessor-level selection should apply to density (one axes)");
  expect(traces(acc.box()) >= 1 && traces(acc.violin()) >= 1, "accessor-level selection should apply to box and violin");
  expect(acc.scatter_matrix().panes().size() == 4, "accessor-level selection should apply to scatter_matrix (2 x 2 grid)");
  expect(traces(three.plot({.exclude_features = {"c"}}).line()) == 2, "accessor-level exclude_features");
  expect(traces(three.plot({.all_features = true}).line("a", {"b", "c"})) == 2, "names override the accessor selection");
  // Per-plot selection overrides the accessor one for that call.
  expect(traces(acc.line({.features = {.include_features = {"c"}}})) == 1, "per-plot selection overrides the accessor");
  expect(traces(acc.line({.features = {.exclude_features = {"a"}}})) == 2, "per-plot exclude overrides the accessor");
  expect(traces(acc.line()) == 2, "the accessor selection is untouched by the override");
  // Names in the call override both.
  expect(traces(acc.line("a", {"c"}, {.features = {.include_features = {"b"}}})) == 1, "names override both selections");
  // Misuse at the accessor level errors with the directive message.
  const auto bad = error_text([&] { (void)three.plot({.all_features = true, .exclude_features = {"a"}}).line(); });
  expect(has(bad, "all_features and exclude_features cannot be combined") && has(bad, "set exclude_features instead"),
         "accessor-level misuse: " + bad);
  // ...but a per-plot selection replaces a misused accessor one entirely.
  expect(traces(three.plot({.all_features = true, .exclude_features = {"a"}}).line({.features = {.include_features = {"b"}}})) == 1,
         "per-plot selection wins even over an invalid accessor-level one");
}

} // namespace

// Several rows can share one x label (a month with a row per city); the axis gets one tick per distinct label.
void test_repeated_x_labels_give_one_tick_each() {
  const auto frame = DataFrame({
      {"month", Shelf(pluss::make_note(std::vector<std::string>{"2024-01", "2024-01", "2024-02", "2024-02", "2024-03", "2024-03"}))},
      {"revenue", Shelf(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 2.0, 1.0, 4.0}))},
  });
  const auto svg = frame.plot().line("month", "revenue").to_svg();
  for (const std::string_view label : {">2024-01</text>", ">2024-02</text>", ">2024-03</text>"}) {
    std::size_t count = 0;
    for (auto at = svg.find(label); at != std::string::npos; at = svg.find(label, at + 1)) {
      ++count;
    }
    expect(count == 1, "a repeated x label should be ticked once");
  }
}

int main() {
  test_repeated_x_labels_give_one_tick_each();
  test_default_columns_and_date_axis();
  test_feature_selection_precedence();
  test_shelf_plot_apis();
  test_object_plot_apis();
  test_scatter_heatmap_and_validation();
  test_scatter_matrix_guardrails();
  test_plot_options_defaults();
  test_corr_methods_and_plot3d();
  return 0;
}
