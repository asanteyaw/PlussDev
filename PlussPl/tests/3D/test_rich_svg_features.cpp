#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

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

std::string read_text(const std::string& path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("failed to read " + path);
  }
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

} // namespace

int main() {
  std::vector<double> strike;
  std::vector<double> expiry;
  for (std::size_t i = 0; i < 10; ++i) {
    strike.push_back(80.0 + static_cast<double>(i) * 5.0);
  }
  for (std::size_t j = 0; j < 8; ++j) {
    expiry.push_back(0.15 + static_cast<double>(j) * 0.2);
  }

  std::vector<std::vector<double>> price_surface(expiry.size(), std::vector<double>(strike.size(), 0.0));
  for (std::size_t r = 0; r < expiry.size(); ++r) {
    for (std::size_t c = 0; c < strike.size(); ++c) {
      const double t = expiry[r];
      const double k = strike[c];
      const double intrinsic = std::max(0.0, 110.0 - k);
      price_surface[r][c] = intrinsic * std::exp(-0.08 * t) + 2.0 * std::sqrt(t) + 0.6 * std::sin(0.1 * k);
    }
  }

  std::vector<double> path_x;
  std::vector<double> path_y;
  std::vector<double> path_z;
  for (std::size_t i = 0; i < expiry.size(); ++i) {
    const double k = 90.0 + static_cast<double>(i) * 2.5;
    const double t = expiry[i];
    path_x.push_back(k);
    path_y.push_back(t);
    path_z.push_back(std::max(0.0, 110.0 - k) * std::exp(-0.08 * t) + 2.0 * std::sqrt(t) + 0.6 * std::sin(0.1 * k));
  }

  pluss::plotism::Style path_style;
  path_style.line_style = "dashed";
  path_style.marker = "circle";
  path_style.line_width = 2.0;
  path_style.color = "#0f172a";

  pluss::plotism::Figure3D fig;
  fig.with_title("rich3d")
      .add_surface(strike, expiry, price_surface, "option surface")
      .add_line3(path_x, path_y, path_z, "simulation path", path_style);
  fig.theme("slate").font_family("IBM Plex Sans");
  fig.view(132.0, 24.0).camera_distance(3.6).projection(pluss::plotism::ProjectionMode::Perspective);
  fig.x_label("strike").y_label("expiry").z_label("price");
  fig.legend(pluss::plotism::Legend{true, pluss::plotism::LegendLocation::OutsideRight, "3D traces"});

  const std::string svg = unique_path("plusspl_test_rich_3d", ".svg");
  fig.save_svg(svg, 980, 720);

  const std::string content = read_text(svg);
  require(content.find("3D traces") != std::string::npos, "expected legend title in 3d svg");
  require(content.find("price") != std::string::npos, "expected z-axis label in 3d svg");
  require(content.find("stroke-dasharray") != std::string::npos, "expected dashed line styling in 3d svg");
  require(content.find("strike") != std::string::npos, "expected x-axis label in 3d svg");
  require(content.find("font-family=\"IBM Plex Sans\"") != std::string::npos, "expected custom font family in 3d svg");
  require(content.find("fill=\"#eef3f8\"") != std::string::npos, "expected slate theme page background in 3d svg");
  require(content.find("polygon") != std::string::npos, "expected surface polygons in 3d svg");

  return 0;
}
