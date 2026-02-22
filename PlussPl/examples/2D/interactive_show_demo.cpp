#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <vector>

#include <fmt/format.h>

#include "pluss/plotism/plotism.h"

int main() {
  std::vector<double> x;
  std::vector<double> y1;
  std::vector<double> y2;
  x.reserve(240);
  y1.reserve(240);
  y2.reserve(240);

  for (std::size_t i = 0; i < 240; ++i) {
    const double xv = static_cast<double>(i) * 0.05;
    x.push_back(xv);
    y1.push_back(std::sin(xv));
    y2.push_back(std::cos(xv));
  }

  pluss::plotism::Figure2D fig;
  fig.with_title("PlussPl 2D Interactive Show Demo")
      .title("sin/cos")
      .x_label("x")
      .y_label("value")
      .add_line(x, y1, "sin(x)")
      .add_line(x, y2, "cos(x)");

  const auto cwd = std::filesystem::current_path();
  const auto svg = (cwd / "plusspl_interactive_2d.svg").string();
  const auto pdf = (cwd / "plusspl_interactive_2d.pdf").string();
  fig.save_svg(svg, 1200, 800);
  fig.save_pdf(pdf, 1200, 800);

  const bool opened = fig.show_window(false);
  std::cout << fmt::format("show_window() opened viewer: {}\n", opened ? "true" : "false");
  std::cout << fmt::format("Saved SVG: {}\n", svg);
  std::cout << fmt::format("Saved PDF: {}\n", pdf);

  std::cout << "Calling fig.show() now. If interactive show is disabled or viewer unavailable, you will see text preview.\n";
  fig.show();

  return 0;
}
