#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

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

  const std::array<int, 3> native_values{10, 20, 30};

  pluss::plotism::Figure2D fig;
  fig.with_title("plussqit-input-types");
  fig.add_line(numeric_note, "note-numeric");
  fig.add_bar(text_note, "note-text");
  fig.add_line(timeline, "timeline-y");
  fig.add_scatter(timeindex, native_values, "timeindex-x-native-y");
  fig.add_hist(std::vector<int>{1, 1, 2, 3, 5, 8}, 4, "native-hist");

  const auto& traces = fig.subplots().front().traces;
  require(traces.size() == 5, "expected 5 traces");

  require(nearly_equal(traces[0].y[0], 1.5), "numeric Note should parse to doubles");
  require(nearly_equal(traces[0].y[1], 2.0), "numeric Note should parse to doubles");
  require(nearly_equal(traces[0].y[2], 3.25), "numeric Note should parse to doubles");

  require(nearly_equal(traces[1].y[0], 5.0), "text Note should fallback to token length");
  require(nearly_equal(traces[1].y[1], 4.0), "text Note should fallback to token length");
  require(nearly_equal(traces[1].y[2], 5.0), "text Note should fallback to token length");

  require(nearly_equal(traces[2].y[0], 0.0), "Timeline should convert to epoch seconds");
  require(nearly_equal(traces[2].y[1], 1.0), "Timeline should convert to epoch seconds");
  require(nearly_equal(traces[2].y[2], 2.0), "Timeline should convert to epoch seconds");

  require(nearly_equal(traces[3].x[0], 0.0), "TimeIndex x should convert to epoch seconds");
  require(nearly_equal(traces[3].x[1], 1.0), "TimeIndex x should convert to epoch seconds");
  require(nearly_equal(traces[3].x[2], 2.0), "TimeIndex x should convert to epoch seconds");
  require(nearly_equal(traces[3].y[2], 30.0), "native std::array<int> should convert to double");

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
