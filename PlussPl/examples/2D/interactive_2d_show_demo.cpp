#include <filesystem>
#include <iostream>

#include "pluss/codem/series_factory.h"
#include "pluss/plot/plot.h"

int main() {
  const auto x = pluss::linspace(0.0, 12.0, 240, pluss::ScalarType::Float64);
  const auto y1 = x.sin();
  const auto y2 = x.cos();
  const auto noise = pluss::randn({240}, pluss::ScalarType::Float64, 42).mul(0.08);

  pluss::plotism::Style sin_style;
  sin_style.color = "#0f766e";
  sin_style.line_style = "dashdot";
  sin_style.marker = "diamond";
  sin_style.line_width = 2.0;

  pluss::plotism::Style cos_style;
  cos_style.color = "#1d4ed8";
  cos_style.line_width = 1.8;

  pluss::plotism::Style scatter_style;
  scatter_style.color = "#c2410c";
  scatter_style.marker = "square";

  pluss::plotism::Figure2D fig;
  fig.with_title("PlussPl 2D Interactive Show Demo")
      .title("sin/cos")
      .x_label("x")
      .y_label("value")
      .legend(pluss::plotism::Legend{true, pluss::plotism::LegendLocation::TopLeft, "Signals"})
      .add_line(x, y1, "sin(x)", {}, sin_style)
      .add_line(x, y2, "cos(x)", {}, cos_style)
      .add_scatter(x, y1.add(noise), "sin(x)+noise", {}, scatter_style);

  const auto cwd = std::filesystem::current_path();
  const auto svg = (cwd / "plusspl_interactive_2d.svg").string();
  const auto pdf = (cwd / "plusspl_interactive_2d.pdf").string();
  fig.save_svg(svg, 1200, 800);
  fig.save_pdf(pdf, 1200, 800);

  const bool opened = fig.show_window(false);
  std::cout << "show_window() opened viewer: " << (opened ? "true" : "false") << "\n";
  std::cout << "Saved SVG: " << svg << "\n";
  std::cout << "Saved PDF: " << pdf << "\n";
  std::cout << "Quick API also works: pluss::plotism::line(pluss::rand({50}, pluss::ScalarType::Float64, 7), \"rand(50)\")\n";

  std::cout << "Calling fig.show() now. If interactive show is disabled or viewer unavailable, you will see text preview.\n";
  fig.show();

  return 0;
}
