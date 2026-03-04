#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "pluss/codem/series_factory.h"
#include "pluss/coden/note_factory.h"
#include "pluss/codeh/timeindex_factory.h"
#include "pluss/codeh/timeline_factory.h"
#include "pluss/plot/plot.h"

namespace {

std::string unique_path(const std::string& stem, const std::string& ext) {
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  return (std::filesystem::temp_directory_path() / (stem + "_" + std::to_string(now) + ext)).string();
}

void require(bool cond, const std::string& msg) {
  if (!cond) {
    throw std::runtime_error(msg);
  }
}

bool nearly_equal(double a, double b, double eps = 1e-9) {
  return std::abs(a - b) <= eps;
}

} // namespace

int main() {
  const auto numeric_note = pluss::make_note({"1.5", "2.0", "3.25"});
  const auto text_note = pluss::make_note({"alpha", "beta", "gamma"});

  const std::vector<std::int64_t> ticks_ns{0LL, 1'000'000'000LL, 2'000'000'000LL};
  const std::vector<std::int64_t> shape{static_cast<std::int64_t>(ticks_ns.size())};
  const auto timeline = pluss::from_epoch_nanos(std::span<const std::int64_t>(ticks_ns.data(), ticks_ns.size()),
                                                 std::span<const std::int64_t>(shape.data(), shape.size()));
  const auto timeindex = pluss::make_timeindex(timeline);

  std::vector<double> numeric_values;
  numeric_values.reserve(static_cast<std::size_t>(numeric_note.numel()));
  for (const auto token : numeric_note) {
    numeric_values.push_back(std::stod(std::string(token)));
  }

  std::vector<double> text_lengths;
  text_lengths.reserve(static_cast<std::size_t>(text_note.numel()));
  for (const auto token : text_note) {
    text_lengths.push_back(static_cast<double>(token.size()));
  }

  const std::vector<double> native_values{10.0, 20.0, 30.0};
  const auto numeric_series = pluss::make_series(numeric_values, {static_cast<std::int64_t>(numeric_values.size())});
  const auto text_series = pluss::make_series(text_lengths, {static_cast<std::int64_t>(text_lengths.size())});
  const auto timeline_series = timeline.epoch_seconds();
  const auto timeindex_series = timeindex.timeline().epoch_seconds();
  const auto native_series = pluss::make_series(native_values, {static_cast<std::int64_t>(native_values.size())});
  const auto hist_series = pluss::make_series(std::vector<double>{1, 1, 2, 3, 5, 8}, {6});

  pluss::plotism::Figure2D fig;
  fig.with_title("series-only-input-types");
  fig.add_line(numeric_series, "note-numeric-series");
  fig.add_bar(text_series, "note-text-series");
  fig.add_line(timeline_series, "timeline-series");
  fig.add_scatter(timeindex_series, native_series, "timeindex-x-series-y");
  fig.add_hist(hist_series, 4, "series-hist");

  const auto& traces = fig.subplots().front().traces;
  require(traces.size() == 5, "expected 5 traces");

  require(nearly_equal(traces[0].y[0], 1.5), "numeric Note should parse to doubles");
  require(nearly_equal(traces[0].y[1], 2.0), "numeric Note should parse to doubles");
  require(nearly_equal(traces[0].y[2], 3.25), "numeric Note should parse to doubles");

  require(nearly_equal(traces[1].y[0], 5.0), "text Note should fallback to token length");
  require(nearly_equal(traces[1].y[1], 4.0), "text Note should fallback to token length");
  require(nearly_equal(traces[1].y[2], 5.0), "text Note should fallback to token length");

  require(nearly_equal(traces[2].y[0], 0.0), "Timeline Series should convert to epoch seconds");
  require(nearly_equal(traces[2].y[1], 1.0), "Timeline Series should convert to epoch seconds");
  require(nearly_equal(traces[2].y[2], 2.0), "Timeline Series should convert to epoch seconds");

  require(nearly_equal(traces[3].x[0], 0.0), "TimeIndex Series x should convert to epoch seconds");
  require(nearly_equal(traces[3].x[1], 1.0), "TimeIndex Series x should convert to epoch seconds");
  require(nearly_equal(traces[3].x[2], 2.0), "TimeIndex Series x should convert to epoch seconds");
  require(nearly_equal(traces[3].y[2], 30.0), "native values should be wrapped into Series");

  const std::string svg = unique_path("plusspl_test_plussqit_inputs", ".svg");
  const std::string pdf = unique_path("plusspl_test_plussqit_inputs", ".pdf");
  fig.save_svg(svg, 900, 600);
  fig.save_pdf(pdf, 900, 600);

  require(std::filesystem::exists(svg), "plussqit-inputs svg file was not created");
  require(std::filesystem::exists(pdf), "plussqit-inputs pdf file was not created");
  require(std::filesystem::file_size(svg) > 0, "plussqit-inputs svg file is empty");
  require(std::filesystem::file_size(pdf) > 0, "plussqit-inputs pdf file is empty");

  return 0;
}
