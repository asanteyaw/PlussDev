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

void test_object_and_kind_dispatch_apis() {
  const auto df = make_xy_frame();

  const auto fig_hist_obj = df.plot().hist("y", 7);
  expect(fig_hist_obj.pane_count() == 1, "plot().hist should produce one pane");
  expect(fig_hist_obj.panes().front().trace_count() == 1, "plot().hist should emit one trace");
  expect(fig_hist_obj.panes().front().traces().front().kind() == TraceKind::Bucket,
         "plot().hist should emit bucket trace");

  const auto fig_hist_kind = df.plot("hist", "y", std::nullopt, std::nullopt, 7);
  expect(fig_hist_kind.pane_count() == 1, "plot(kind=hist) should produce one pane");
  expect(fig_hist_kind.panes().front().trace_count() == 1, "plot(kind=hist) should emit one trace");
  expect(fig_hist_kind.panes().front().traces().front().kind() == TraceKind::Bucket,
         "plot(kind=hist) should emit bucket trace");

  const auto fig_line_kind = df.plot("line", {"y", "z"}, "x");
  expect(fig_line_kind.panes().front().trace_count() == 2, "plot(kind=line) should emit one trace per y feature");
  expect(fig_line_kind.panes().front().traces().front().kind() == TraceKind::Trend,
         "plot(kind=line) should emit trend traces");

  const auto fig_line_dispatch = df.plot().kind("line", "y", "x");
  expect(fig_line_dispatch.panes().front().trace_count() == 1,
         "plot().kind(line, single feature) should emit one trace");
  expect(fig_line_dispatch.panes().front().traces().front().kind() == TraceKind::Trend,
         "plot().kind(line, single feature) should emit trend trace");
}

void test_scatter_heatmap_and_kind_validation() {
  const auto df = make_xy_frame();
  const auto fig_scatter = df.plot("scatter", {"x", "y"}, std::nullopt, "grp");
  expect(fig_scatter.panes().front().trace_count() == 2, "plot(kind=scatter,hue) should split by hue");
  expect(fig_scatter.panes().front().traces().front().kind() == TraceKind::Cloud,
         "plot(kind=scatter) should emit cloud trace");

  const auto surface = DataFrame({
      {"maturity", Thread(pluss::make_note({"1M", "1M", "3M", "3M"}))},
      {"delta_bucket", Thread(pluss::make_note({"25P", "ATM", "25P", "ATM"}))},
      {"implied_vol", Thread(pluss::make_series(std::vector<double>{0.30, 0.25, 0.33, 0.28}))},
  });
  const auto fig_heatmap = surface.plot("heatmap", {"maturity", "delta_bucket", "implied_vol"});
  expect(fig_heatmap.panes().front().trace_count() == 1, "plot(kind=heatmap,pivot) should emit one trace");
  expect(fig_heatmap.panes().front().traces().front().kind() == TraceKind::Fieldmap,
         "plot(kind=heatmap,pivot) should emit fieldmap trace");

  expect_throw([&]() { (void)df.plot("unsupported_kind", {"x"}); },
               "plot(kind=unsupported) should throw");
}

} // namespace

int main() {
  test_object_and_kind_dispatch_apis();
  test_scatter_heatmap_and_kind_validation();
  return 0;
}
