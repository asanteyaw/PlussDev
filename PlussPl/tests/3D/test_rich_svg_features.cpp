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
  std::vector<double> x;
  std::vector<double> y;
  std::vector<double> z;
  for (std::size_t i = 0; i < 60; ++i) {
    const double t = static_cast<double>(i) * 0.12;
    x.push_back(std::cos(t));
    y.push_back(std::sin(t));
    z.push_back(0.08 * static_cast<double>(i));
  }

  pluss::plotism::Style helix_style;
  helix_style.color = "#7c3aed";
  helix_style.line_style = "dashed";
  helix_style.marker = "circle";
  helix_style.line_width = 2.0;

  auto fig = pluss::plotism::line3(x, y, z, "helix", helix_style).with_title("rich3d");
  fig.x_label("x-axis").y_label("y-axis").z_label("height");
  fig.legend(pluss::plotism::Legend{true, pluss::plotism::LegendLocation::TopRight, "3D traces"});

  const std::string svg = unique_path("plusspl_test_rich_3d", ".svg");
  fig.save_svg(svg, 980, 720);

  const std::string content = read_text(svg);
  require(content.find("3D traces") != std::string::npos, "expected legend title in 3d svg");
  require(content.find("height") != std::string::npos, "expected z-axis label in 3d svg");
  require(content.find("stroke-dasharray") != std::string::npos, "expected dashed line styling in 3d svg");
  require(content.find("x-axis") != std::string::npos, "expected x-axis label in 3d svg");

  return 0;
}
