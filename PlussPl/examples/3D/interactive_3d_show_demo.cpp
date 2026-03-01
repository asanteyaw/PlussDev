#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <vector>

#include "pluss/plot/plot.h"

int main() {
  std::vector<double> strike;
  std::vector<double> maturity;
  strike.reserve(17);
  maturity.reserve(12);
  for (std::size_t i = 0; i < 17; ++i) {
    strike.push_back(70.0 + static_cast<double>(i) * 5.0);
  }
  for (std::size_t j = 0; j < 12; ++j) {
    maturity.push_back(0.1 + static_cast<double>(j) * 0.2);
  }

  std::vector<std::vector<double>> iv_surface(maturity.size(), std::vector<double>(strike.size(), 0.0));
  for (std::size_t r = 0; r < maturity.size(); ++r) {
    for (std::size_t c = 0; c < strike.size(); ++c) {
      const double m = maturity[r];
      const double k = strike[c];
      const double moneyness = (k - 100.0) / 30.0;
      iv_surface[r][c] = 0.15 + 0.11 * moneyness * moneyness + 0.035 * std::sqrt(m) + 0.01 * std::sin(2.3 * m + 0.7 * moneyness);
    }
  }

  std::vector<double> sim_x;
  std::vector<double> sim_y;
  std::vector<double> sim_z;
  sim_x.reserve(maturity.size());
  sim_y.reserve(maturity.size());
  sim_z.reserve(maturity.size());
  for (std::size_t i = 0; i < maturity.size(); ++i) {
    const double t = maturity[i];
    const double k = 88.0 + static_cast<double>(i) * 2.5;
    const double moneyness = (k - 100.0) / 30.0;
    sim_x.push_back(k);
    sim_y.push_back(t);
    sim_z.push_back(0.15 + 0.11 * moneyness * moneyness + 0.035 * std::sqrt(t) + 0.01 * std::sin(2.3 * t + 0.7 * moneyness));
  }

  pluss::plotism::Style path_style;
  path_style.line_style = "dashed";
  path_style.marker = "circle";
  path_style.line_width = 2.0;
  path_style.color = "#0f172a";

  pluss::plotism::Figure3D fig;
  fig.with_title("PlussPl 3D Interactive Show Demo")
      .add_surface(strike, maturity, iv_surface, "IV surface")
      .add_line3(sim_x, sim_y, sim_z, "simulation path", path_style);
  fig.theme("slate").font_family("IBM Plex Sans");
  fig.view(128.0, 26.0).camera_distance(3.7).projection(pluss::plotism::ProjectionMode::Perspective);
  fig.x_label("strike").y_label("expiry").z_label("implied vol");
  fig.legend(pluss::plotism::Legend{true, pluss::plotism::LegendLocation::OutsideRight, "3D traces"});

  const auto cwd = std::filesystem::current_path();
  const auto svg = (cwd / "plusspl_interactive_3d.svg").string();
  const auto pdf = (cwd / "plusspl_interactive_3d.pdf").string();
  fig.save_svg(svg, 1200, 900);
  fig.save_pdf(pdf, 1200, 900);

  const bool opened = fig.show_window(false);
  std::cout << "show_window() opened viewer: " << (opened ? "true" : "false") << "\n";
  std::cout << "Saved SVG: " << svg << "\n";
  std::cout << "Saved PDF: " << pdf << "\n";

  std::cout << "Calling fig.show() now. If interactive show is disabled or viewer unavailable, you will see text preview.\n";
  fig.show();

  return 0;
}
