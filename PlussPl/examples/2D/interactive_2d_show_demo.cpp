#include <filesystem>
#include <iostream>

#include "../output_paths.h"
#include "pluss/codem/series_factory.h"
#include "pluss/plot/plot.h"

int main() {
  const auto x = pluss::linspace(0.0, 12.0, 240, pluss::ScalarType::Float64);
  const auto y1 = x.sin();
  const auto y2 = x.cos();
  const auto noise = pluss::randn({240}, pluss::ScalarType::Float64, 42).mul(0.08);

  pluss::plotism::Figure2D fig;
  fig.theme("paper").font_family("IBM Plex Sans");
  fig.subplot(2, 2, 0)
      .with_title("PlussPl 2D Interactive Show Demo")
      .title("Minimal syntax")
      .x_label("x")
      .y_label("signal")
      .legend(pluss::plotism::Legend{true, pluss::plotism::LegendLocation::OutsideRight, "Quick plots"})
      .add_area(x, y1, "signal")
      .add_band(x, y1.sub(0.18), y1.add(0.18), "band")
      .add_step(x, y2, "step")
      .annotate("crest", 8.8, 0.58);

  fig.subplot(2, 2, 1)
      .title("Scatter + line")
      .x_label("x")
      .y_label("value")
      .add_line(x, y2, "cos(x)")
      .add_scatter(x, y1.add(noise), "noisy")
      .add_errorbar({1.0, 3.0, 5.0, 7.0},
                    {0.82, 0.15, -0.62, 0.48},
                    {0.10, 0.16, 0.12, 0.14},
                    "err");

  fig.subplot(2, 2, 2)
      .title("Heatmap")
      .x_label("quarter")
      .y_label("desk")
      .colorbar(pluss::plotism::Colorbar{true, pluss::plotism::ColorbarLocation::Right, "load"})
      .add_heatmap({{0.2, 0.4, 0.7, 0.9},
                    {0.1, 0.3, 0.6, 0.8},
                    {-0.1, 0.2, 0.5, 0.7}},
                   "load",
                   {"Q1", "Q2", "Q3", "Q4"},
                   {"A", "B", "C"});

  fig.subplot(2, 2, 3)
      .title("Tiny code, more plot")
      .x_label("index")
      .y_label("value")
      .add_bar({2.0, 3.5, 1.8, 4.2}, "bars", {"one", "two", "three", "four"})
      .annotate("best", 3.0, 4.2);

  const auto svg = plusspl::examples::output_path(__FILE__, "plusspl_interactive_2d.svg");
  const auto png = plusspl::examples::output_path(__FILE__, "plusspl_interactive_2d.png");
  const auto pdf = plusspl::examples::output_path(__FILE__, "plusspl_interactive_2d.pdf");
  fig.save_svg(svg, 1200, 800);
  fig.save_png(png, 1200, 800);
  fig.save_pdf(pdf, 1200, 800);

  const bool opened = fig.show_window(false);
  std::cout << "show_window() opened viewer: " << (opened ? "true" : "false") << "\n";
  std::cout << "Saved SVG: " << svg << "\n";
  std::cout << "Saved PNG: " << png << "\n";
  std::cout << "Saved PDF: " << pdf << "\n";
  std::cout << "Quick API also works: pluss::plotism::heatmap({{1.0, 2.0}, {3.0, 4.0}})\n";

  std::cout << "Calling fig.show() now. If interactive show is disabled or viewer unavailable, you will see text preview.\n";
  fig.show();

  return 0;
}
