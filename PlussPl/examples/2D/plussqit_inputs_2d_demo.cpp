#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <span>
#include <string>
#include <vector>

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

  const std::array<int, 4> native_y{10, 20, 35, 50};

  pluss::plotism::Figure2D fig;
  fig.with_title("PlussQit Input Types Demo")
      .title("Note + Timeline + TimeIndex + Native C++")
      .x_label("x")
      .y_label("value")
      .add_line(numeric_note, "Note numeric")
      .add_bar(word_note, "Note fallback:length")
      .add_line(timeline, "Timeline->epoch_seconds")
      .add_scatter(timeindex, native_y, "TimeIndex x + std::array y")
      .add_hist(std::vector<int>{1, 1, 2, 3, 5, 8, 13, 21}, 6, "std::vector<int> hist");

  const auto cwd = std::filesystem::current_path();
  const auto svg = (cwd / "plusspl_inputs_2d_demo.svg").string();
  const auto pdf = (cwd / "plusspl_inputs_2d_demo.pdf").string();
  fig.save_svg(svg, 1200, 800);
  fig.save_pdf(pdf, 1200, 800);

  std::cout << "Saved SVG: " << svg << "\n";
  std::cout << "Saved PDF: " << pdf << "\n";
  std::cout << "show_window() opened viewer: " << (fig.show_window(false) ? "true" : "false") << "\n";
  fig.show();

  return 0;
}
