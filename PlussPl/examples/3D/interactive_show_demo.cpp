#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <vector>

#include <fmt/format.h>

#include "pluss/plotism/plotism.h"

int main() {
  std::vector<double> x;
  std::vector<double> y;
  std::vector<double> z;
  x.reserve(180);
  y.reserve(180);
  z.reserve(180);

  for (std::size_t i = 0; i < 180; ++i) {
    const double t = static_cast<double>(i) * 0.08;
    x.push_back(std::cos(t));
    y.push_back(std::sin(t));
    z.push_back(0.12 * t);
  }

  auto fig = pluss::plotism::line3(x, y, z, "helix").with_title("PlussPl 3D Interactive Show Demo");
  fig.x_label("x").y_label("y").z_label("z");

  const auto cwd = std::filesystem::current_path();
  const auto svg = (cwd / "plusspl_interactive_3d.svg").string();
  const auto pdf = (cwd / "plusspl_interactive_3d.pdf").string();
  fig.save_svg(svg, 1200, 900);
  fig.save_pdf(pdf, 1200, 900);

  const bool opened = fig.show_window(false);
  std::cout << fmt::format("show_window() opened viewer: {}\n", opened ? "true" : "false");
  std::cout << fmt::format("Saved SVG: {}\n", svg);
  std::cout << fmt::format("Saved PDF: {}\n", pdf);

  std::cout << "Calling fig.show() now. If interactive show is disabled or viewer unavailable, you will see text preview.\n";
  fig.show();

  return 0;
}
