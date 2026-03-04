#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>

#include "../output_paths.h"
#include "pluss/plot/plot.h"

int main() {
  std::vector<double> strike;
  std::vector<double> expiry;
  strike.reserve(17);
  expiry.reserve(11);
  for (std::size_t i = 0; i < 17; ++i) {
    strike.push_back(75.0 + static_cast<double>(i) * 4.0);
  }
  for (std::size_t j = 0; j < 11; ++j) {
    expiry.push_back(0.15 + static_cast<double>(j) * 0.22);
  }

  std::vector<std::vector<double>> iv(expiry.size(), std::vector<double>(strike.size(), 0.0));
  for (std::size_t r = 0; r < expiry.size(); ++r) {
    for (std::size_t c = 0; c < strike.size(); ++c) {
      const double t = expiry[r];
      const double k = strike[c];
      const double x = (k - 102.0) / 26.0;
      iv[r][c] = 0.16 + 0.09 * x * x + 0.028 * std::sqrt(t) + 0.008 * std::cos(2.0 * t + 1.2 * x);
    }
  }

  std::vector<double> path_x;
  std::vector<double> path_y;
  std::vector<double> path_z;
  for (std::size_t i = 0; i < expiry.size(); ++i) {
    const double t = expiry[i];
    const double k = 84.0 + static_cast<double>(i) * 3.0;
    const double x = (k - 102.0) / 26.0;
    path_x.push_back(k);
    path_y.push_back(t);
    path_z.push_back(0.16 + 0.09 * x * x + 0.028 * std::sqrt(t) + 0.008 * std::cos(2.0 * t + 1.2 * x));
  }

  pluss::plotism::Style path_style;
  path_style.line_style = "dashed";
  path_style.marker = "circle";
  path_style.line_width = 2.0;
  path_style.color = "#0f172a";

  pluss::plotism::Figure3D fig;
  fig.with_title("PlussPl 3D Popup Viewer Demo")
      .add_surface(strike, expiry, iv, "IV surface")
      .surface_contours(true, 2)
      .add_line3(path_x, path_y, path_z, "simulation path", path_style);
  fig.theme("slate").font_family("IBM Plex Sans");
  fig.view(128.0, 24.0).camera_distance(3.6).projection(pluss::plotism::ProjectionMode::Perspective);
  fig.x_label("strike").y_label("expiry").z_label("implied vol");
  fig.legend(pluss::plotism::Legend{true, pluss::plotism::LegendLocation::OutsideRight, "3D traces"});
  fig.colorbar(pluss::plotism::Colorbar{true, pluss::plotism::ColorbarLocation::Right, "implied vol"});

  const auto svg = plusspl::examples::output_path(__FILE__, "plusspl_popup_3d.svg");
  const auto png = plusspl::examples::output_path(__FILE__, "plusspl_popup_3d.png");
  const auto pdf = plusspl::examples::output_path(__FILE__, "plusspl_popup_3d.pdf");
  fig.save_svg(svg, 1280, 920);
  fig.save_png(png, 1280, 920);
  fig.save_pdf(pdf, 1280, 920);

  std::cout << "Saved SVG: " << svg << "\n";
  std::cout << "Saved PNG: " << png << "\n";
  std::cout << "Saved PDF: " << pdf << "\n";
  std::cout << "Opening blocking popup viewer. Close the window to return.\n";
  const bool opened = fig.show_popup(pluss::plotism::OutputFormat::Svg, 1280, 920);
  std::cout << "show_popup() opened viewer: " << (opened ? "true" : "false") << "\n";

  return 0;
}
