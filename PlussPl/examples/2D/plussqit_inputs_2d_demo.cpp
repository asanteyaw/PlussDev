#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "pluss/codem/series_factory.h"
#include "../output_paths.h"
#include "pluss/coden/note_factory.h"
#include "pluss/codeh/timeindex_factory.h"
#include "pluss/codeh/timeline_factory.h"
#include "pluss/plot/plot.h"

int main() {
  const auto numeric_note = pluss::make_note({"1.0", "2.5", "3.25", "4.75"});
  const auto word_note = pluss::make_note({"alpha", "beta", "gamma", "delta"});

  const std::vector<std::int64_t> ticks_ns{
      0LL, 1'000'000'000LL, 2'000'000'000LL, 3'000'000'000LL};
  const std::vector<std::int64_t> shape{static_cast<std::int64_t>(ticks_ns.size())};
  const auto timeline = pluss::from_epoch_nanos(std::span<const std::int64_t>(ticks_ns.data(), ticks_ns.size()),
                                                 std::span<const std::int64_t>(shape.data(), shape.size()));
  const auto timeindex = pluss::make_timeindex(timeline);

  std::vector<double> numeric_note_values;
  numeric_note_values.reserve(static_cast<std::size_t>(numeric_note.numel()));
  for (const auto token : numeric_note) {
    numeric_note_values.push_back(std::stod(std::string(token)));
  }

  std::vector<double> word_note_lengths;
  word_note_lengths.reserve(static_cast<std::size_t>(word_note.numel()));
  for (const auto token : word_note) {
    word_note_lengths.push_back(static_cast<double>(token.size()));
  }

  const std::vector<double> native_y{10.0, 20.0, 35.0, 50.0};
  const auto numeric_note_series = pluss::make_series(numeric_note_values, {static_cast<std::int64_t>(numeric_note_values.size())});
  const auto word_note_series = pluss::make_series(word_note_lengths, {static_cast<std::int64_t>(word_note_lengths.size())});
  const auto timeline_series = timeline.epoch_seconds();
  const auto timeindex_series = timeindex.timeline().epoch_seconds();
  const auto native_y_series = pluss::make_series(native_y, {static_cast<std::int64_t>(native_y.size())});
  const auto hist_series = pluss::make_series(std::vector<double>{1, 1, 2, 3, 5, 8, 13, 21}, {8});

  pluss::plotism::Figure2D fig;
  fig.with_title("Series-Only Plot Inputs Demo")
      .title("Upstream inputs converted to Series before plotting")
      .x_label("x")
      .y_label("value")
      .add_line(numeric_note_series, "Note numeric -> Series")
      .add_bar(word_note_series, "Note length -> Series")
      .add_line(timeline_series, "Timeline seconds -> Series")
      .add_scatter(timeindex_series, native_y_series, "TimeIndex seconds + Series y")
      .add_hist(hist_series, 6, "Series hist");

  const auto svg = plusspl::examples::output_path(__FILE__, "plusspl_inputs_2d_demo.svg");
  const auto png = plusspl::examples::output_path(__FILE__, "plusspl_inputs_2d_demo.png");
  const auto pdf = plusspl::examples::output_path(__FILE__, "plusspl_inputs_2d_demo.pdf");
  fig.save_svg(svg, 1200, 800);
  fig.save_png(png, 1200, 800);
  fig.save_pdf(pdf, 1200, 800);

  std::cout << "Saved SVG: " << svg << "\n";
  std::cout << "Saved PNG: " << png << "\n";
  std::cout << "Saved PDF: " << pdf << "\n";
  std::cout << "show_window() opened viewer: " << (fig.show_window(false) ? "true" : "false") << "\n";
  fig.show();

  return 0;
}
