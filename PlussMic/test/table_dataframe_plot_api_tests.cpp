#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::pl::TraceKind;
using pluss::table::DataFrame;
using pluss::table::Thread;

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
      {"x", Thread(pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}))},
      {"y", Thread(pluss::make_series(std::vector<double>{1.0, 1.4, 2.2, 3.0}))},
      {"z", Thread(pluss::make_series(std::vector<double>{0.8, 1.0, 1.6, 2.4}))},
      {"grp", Thread(pluss::make_note({"alpha", "alpha", "beta", "beta"}))},
  });
}

void test_object_plot_apis() {
  const auto df = make_xy_frame();

  const auto fig_hist_obj = df.plot().hist("y", 7);
  expect(fig_hist_obj.pane_count() == 1, "plot().hist should produce one pane");
  expect(fig_hist_obj.panes().front().trace_count() == 1, "plot().hist should emit one trace");
  expect(fig_hist_obj.panes().front().traces().front().kind() == TraceKind::Bucket,
         "plot().hist should emit bucket trace");

  const auto fig_line = df.plot().line({"y", "z"}, "x");
  expect(fig_line.panes().front().trace_count() == 2, "plot().line should emit one trace per y feature");
  expect(fig_line.panes().front().traces().front().kind() == TraceKind::Trend,
         "plot().line should emit trend traces");

  const auto fig_violin = df.plot().violin({"y", "z"});
  expect(fig_violin.panes().front().trace_count() == 1, "plot().violin(features) should emit one trace");
  expect(fig_violin.panes().front().traces().front().kind() == TraceKind::Violin,
         "plot().violin(features) should emit violin trace");

  const auto fig_violin_grouped = df.plot().violin("y", "grp");
  expect(fig_violin_grouped.panes().front().trace_count() == 1, "plot().violin(value, group) should emit one trace");
  expect(fig_violin_grouped.panes().front().traces().front().kind() == TraceKind::Violin,
         "plot().violin(value, group) should emit violin trace");
}

void test_scatter_heatmap_and_validation() {
  const auto df = make_xy_frame();
  const auto fig_scatter = df.plot().scatter("x", "y", "grp");
  expect(fig_scatter.panes().front().trace_count() == 2, "plot().scatter(hue) should split by hue");
  expect(fig_scatter.panes().front().traces().front().kind() == TraceKind::Cloud,
         "plot().scatter should emit cloud trace");

  const auto surface = DataFrame({
      {"maturity", Thread(pluss::make_note({"1M", "1M", "3M", "3M"}))},
      {"delta_bucket", Thread(pluss::make_note({"25P", "ATM", "25P", "ATM"}))},
      {"implied_vol", Thread(pluss::make_series(std::vector<double>{0.30, 0.25, 0.33, 0.28}))},
  });
  const auto fig_heatmap = surface.plot().heatmap("maturity", "delta_bucket", "implied_vol");
  expect(fig_heatmap.panes().front().trace_count() == 1, "plot().heatmap(pivot) should emit one trace");
  expect(fig_heatmap.panes().front().traces().front().kind() == TraceKind::Fieldmap,
         "plot().heatmap(pivot) should emit fieldmap trace");
  expect(fig_heatmap.panes().front().scale_guide_enabled(),
         "plot().heatmap(pivot) should enable scale guide by default");

  const auto fig_corr = df.plot().corr_heatmap({"x", "y", "z"});
  expect(fig_corr.panes().front().traces().front().annotate_levels(),
         "plot().corr_heatmap should annotate cell values by default");

  expect_throw([&]() { (void)df.plot().scatter("x", "missing_feature"); },
               "plot().scatter with missing feature should throw");
}

void test_plot_options_defaults() {
  const auto df = make_xy_frame();
  pluss::table::DataFramePlotOptions options;
  options.default_hue_feature = "grp";
  options.default_bins = 9;
  options.width = 920.0;
  options.height = 640.0;
  options.use_tight_layout = true;

  const auto fig_scatter = df.plot(options).scatter("x", "y");
  expect(fig_scatter.panes().front().trace_count() == 2, "plot(options).scatter should use default hue feature");
  expect(fig_scatter.extent().width == 920.0 && fig_scatter.extent().height == 640.0,
         "plot(options) should apply figure size defaults");

  const auto fig_hist = df.plot(options).hist("y");
  expect(fig_hist.panes().front().trace_count() == 1, "plot(options).hist should still emit one trace");
  expect(fig_hist.panes().front().traces().front().kind() == TraceKind::Bucket,
         "plot(options).hist should emit bucket trace");

  options.bar_ci_enabled = true;
  options.bar_ci_level = 0.95;
  const auto fig_bar_ci = df.plot(options).bar("y", "grp");
  expect(fig_bar_ci.panes().front().trace_count() == 2, "plot(options).bar with CI should emit bar + whisker");
  const auto& traces = fig_bar_ci.panes().front().traces();
  const bool has_whisk = std::any_of(traces.begin(), traces.end(), [](const auto& trace) {
    return trace.kind() == TraceKind::Whisk;
  });
  expect(has_whisk, "plot(options).bar with CI should include whisk trace");

  options.scatter_matrix_triangle = pluss::table::ScatterMatrixTriangle::Lower;
  options.scatter_matrix_auto_tick_decimals = 1;
  const auto fig_corner = df.plot(options).scatter_matrix({"x", "y", "z"});
  std::size_t trace_count = 0;
  for (const auto& pane : fig_corner.panes()) {
    trace_count += pane.trace_count();
  }
  expect(trace_count == 6, "plot(options).scatter_matrix(lower) should render only lower triangle + diagonal");
  expect(fig_corner.panes().front().x_axis().auto_tick_decimals.has_value(),
         "plot(options).scatter_matrix should propagate auto tick decimals");
  expect(*fig_corner.panes().front().x_axis().auto_tick_decimals == 1,
         "plot(options).scatter_matrix should use requested auto tick decimals");

  options.heatmap_annotate_levels = true;
  const auto fig_heatmap = df.plot(options).heatmap({"x", "y"});
  expect(fig_heatmap.panes().front().traces().front().annotate_levels(),
         "plot(options).heatmap should honor heatmap_annotate_levels");
}

void test_corr_methods_and_plot3d() {
  const auto df = make_xy_frame();
  const auto pearson = df.corr({"x", "y", "z"}, false);
  const auto spearman = df.corr({"x", "y", "z"}, DataFrame::CorrMethod::Spearman, false);
  const auto kendall = df.corr({"x", "y", "z"}, DataFrame::CorrMethod::KendallTau, false);
  expect(pearson.ncases() == 3 && pearson.nfeats() == 3, "corr(pearson) should return square matrix");
  expect(spearman.ncases() == 3 && spearman.nfeats() == 3, "corr(spearman) should return square matrix");
  expect(kendall.ncases() == 3 && kendall.nfeats() == 3, "corr(kendall) should return square matrix");

  const auto fig_scatter3 = df.plot3d().scatter("x", "y", "z");
  expect(fig_scatter3.panes().front().traces().front().kind() == TraceKind::Cloud3,
         "plot3d().scatter should emit cloud3 trace");

  const auto fig_line3 = df.plot3d().line("x", "y", "z");
  expect(fig_line3.panes().front().traces().front().kind() == TraceKind::Path3,
         "plot3d().line should emit path3 trace");

  const auto grid = DataFrame({
      {"gx", Thread(pluss::make_series(std::vector<double>{0.0, 1.0, 0.0, 1.0}))},
      {"gy", Thread(pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}))},
      {"gz", Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}))},
  });
  const auto fig_surface = grid.plot3d().surface("gx", "gy", "gz");
  expect(fig_surface.panes().front().traces().front().kind() == TraceKind::Sheet,
         "plot3d().surface should emit sheet trace");

  const auto fig_wire = grid.plot3d().wireframe("gx", "gy", "gz");
  expect(fig_wire.panes().front().traces().front().kind() == TraceKind::Lattice,
         "plot3d().wireframe should emit lattice trace");
}

} // namespace

int main() {
  test_object_plot_apis();
  test_scatter_heatmap_and_validation();
  test_plot_options_defaults();
  test_corr_methods_and_plot3d();
  return 0;
}
