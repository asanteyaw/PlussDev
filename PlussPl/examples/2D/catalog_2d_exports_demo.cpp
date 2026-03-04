#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "../output_paths.h"
#include "pluss/plot/plot.h"

namespace {

std::vector<double> make_range(double start, double stop, std::size_t count) {
  std::vector<double> values;
  values.reserve(count);
  if (count == 0) {
    return values;
  }
  if (count == 1) {
    values.push_back(start);
    return values;
  }
  const double step = (stop - start) / static_cast<double>(count - 1);
  for (std::size_t i = 0; i < count; ++i) {
    values.push_back(start + step * static_cast<double>(i));
  }
  return values;
}

std::vector<double> make_hist_sample() {
  std::vector<double> values;
  values.reserve(180);
  for (std::size_t i = 0; i < 180; ++i) {
    const double t = static_cast<double>(i) / 18.0;
    values.push_back(1.2 * std::sin(t) + 0.45 * std::cos(2.7 * t) + 0.15 * std::sin(6.0 * t));
  }
  return values;
}

} // namespace

int main() {
  using namespace pluss::plotism;

  const auto x = make_range(0.0, 12.0, 160);
  const std::vector<std::string> dates{
      "2026-01", "2026-02", "2026-03", "2026-04", "2026-05", "2026-06", "2026-07", "2026-08"};

  std::vector<double> sin_wave;
  std::vector<double> scatter_y;
  std::vector<double> band_lo;
  std::vector<double> band_hi;
  std::vector<double> area_y;
  std::vector<double> step_y;
  sin_wave.reserve(x.size());
  scatter_y.reserve(x.size());
  band_lo.reserve(x.size());
  band_hi.reserve(x.size());
  area_y.reserve(x.size());
  step_y.reserve(x.size());

  for (double xv : x) {
    const double base = std::sin(xv);
    sin_wave.push_back(base);
    scatter_y.push_back(base + 0.18 * std::sin(4.0 * xv));
    band_lo.push_back(base - 0.22 - 0.04 * std::cos(xv));
    band_hi.push_back(base + 0.22 + 0.04 * std::cos(xv));
    area_y.push_back(0.45 + 0.35 * std::sin(0.6 * xv) + 0.14 * std::cos(1.35 * xv));
    step_y.push_back(1.0 + std::floor((std::sin(0.8 * xv) + 1.2) * 2.0));
  }

  const std::vector<double> month_x{0, 1, 2, 3, 4, 5, 6, 7};
  const std::vector<double> month_y{102.0, 106.0, 104.0, 111.0, 117.0, 121.0, 119.0, 126.0};
  const std::vector<double> bar_x{0, 1, 2, 3, 4};
  const std::vector<double> bar_y{3.2, 5.1, 4.4, 6.0, 4.9};
  const std::vector<std::string> bar_labels{"alpha", "beta", "gamma", "delta", "epsilon"};
  const std::vector<double> err_x{1.0, 2.5, 4.0, 5.5, 7.0};
  const std::vector<double> err_y{1.3, 1.9, 1.4, 2.2, 1.8};
  const std::vector<double> err_v{0.18, 0.24, 0.15, 0.28, 0.21};
  const std::vector<double> hist_values = make_hist_sample();
  const std::vector<std::vector<double>> heatmap{
      {0.15, 0.18, 0.23, 0.28, 0.36},
      {0.20, 0.26, 0.31, 0.35, 0.42},
      {0.27, 0.33, 0.39, 0.45, 0.52},
      {0.35, 0.41, 0.47, 0.54, 0.61}};

  Figure2D fig(5, 2);
  fig.with_title("PlussPl 2D Plot Catalog")
      .theme("paper")
      .font_family("IBM Plex Sans");

  fig.subplot(5, 2, 0)
      .title("Line")
      .x_label("x")
      .y_label("signal")
      .add_line(x, sin_wave, "sin(x)")
      .annotate("crest", 7.9, 0.99, 8.8, 1.15);

  fig.subplot(5, 2, 1)
      .title("Time-style line")
      .x_label("month")
      .y_label("index")
      .add_line(month_x, month_y, "index", dates)
      .add_scatter(month_x, month_y, "marks");

  fig.subplot(5, 2, 2)
      .title("Scatter")
      .x_label("x")
      .y_label("sample")
      .add_scatter(x, scatter_y, "observations")
      .add_line(x, sin_wave, "reference");

  fig.subplot(5, 2, 3)
      .title("Bar")
      .x_label("desk")
      .y_label("volume")
      .add_bar(bar_x, bar_y, "volume", bar_labels);

  fig.subplot(5, 2, 4)
      .title("Histogram")
      .x_label("value")
      .y_label("count")
      .add_hist(hist_values, 18, "distribution");

  fig.subplot(5, 2, 5)
      .title("Step")
      .x_label("x")
      .y_label("bucket")
      .add_step(x, step_y, "state");

  fig.subplot(5, 2, 6)
      .title("Area")
      .x_label("x")
      .y_label("level")
      .add_area(x, area_y, "filled area");

  fig.subplot(5, 2, 7)
      .title("Band")
      .x_label("x")
      .y_label("interval")
      .add_band(x, band_lo, band_hi, "confidence band")
      .add_line(x, sin_wave, "center");

  fig.subplot(5, 2, 8)
      .title("Error bars")
      .x_label("time")
      .y_label("estimate")
      .add_errorbar(err_x, err_y, err_v, "estimate")
      .add_line(err_x, err_y, "mean");

  fig.subplot(5, 2, 9)
      .title("Heatmap")
      .x_label("quarter")
      .y_label("desk")
      .colorbar(Colorbar{.visible = true, .location = ColorbarLocation::Right, .title = "load"})
      .add_heatmap(heatmap, "load", {"Q1", "Q2", "Q3", "Q4", "Q5"}, {"A", "B", "C", "D"});

  const auto svg = plusspl::examples::output_path(__FILE__, "plusspl_2d_catalog.svg");
  const auto png = plusspl::examples::output_path(__FILE__, "plusspl_2d_catalog.png");
  const auto pdf = plusspl::examples::output_path(__FILE__, "plusspl_2d_catalog.pdf");
  fig.save_svg(svg, 1600, 2200);
  fig.save_png(png, 1600, 2200);
  fig.save_pdf(pdf, 1600, 2200);

  std::cout << "Saved SVG: " << svg << "\n";
  std::cout << "Saved PNG: " << png << "\n";
  std::cout << "Saved PDF: " << pdf << "\n";
  std::cout << "Catalog includes implemented 2D plot kinds: line, scatter, bar, hist, step, area, band, errorbar, heatmap.\n";

  return 0;
}
