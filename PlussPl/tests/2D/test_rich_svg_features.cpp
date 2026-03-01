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
  std::vector<double> x(8);
  std::vector<double> y1(8);
  std::vector<double> y2(8);
  for (std::size_t i = 0; i < x.size(); ++i) {
    x[i] = static_cast<double>(i);
    y1[i] = 1.5 + std::sin(static_cast<double>(i) * 0.45);
    y2[i] = 1.2 + std::cos(static_cast<double>(i) * 0.35);
  }

  pluss::plotism::Figure2D fig;
  fig.theme("paper").font_family("IBM Plex Sans");
  fig.subplot(1, 2, 0)
      .with_title("rich2d")
      .title("Area, band, step")
      .x_label("quarter")
      .y_label("value")
      .legend(pluss::plotism::Legend{true, pluss::plotism::LegendLocation::OutsideRight, "Signals"})
      .add_area(x, y1, "area")
      .add_band(x, y1, y2, "band")
      .add_step(x, y2, "step", {"Q1", "Q2", "Q3", "Q4", "Q5", "Q6", "Q7", "Q8"})
      .add_errorbar({1.0, 3.0, 5.0}, {1.6, 2.1, 1.8}, {0.12, 0.18, 0.15}, "err")
      .annotate("crest", 5.0, y2[5]);

  fig.subplot(1, 2, 1)
      .title("Heatmap")
      .x_label("quarter")
      .y_label("desk")
      .colorbar(pluss::plotism::Colorbar{true, pluss::plotism::ColorbarLocation::Right, "Intensity"})
      .add_heatmap({{0.1, 0.4, 0.8},
                    {0.2, 0.5, 0.9},
                    {-0.1, 0.3, 0.7}},
                   "load",
                   {"Q1", "Q2", "Q3"},
                   {"A", "B", "C"});

  const std::string svg = unique_path("plusspl_test_rich_2d", ".svg");
  fig.save_svg(svg, 980, 640);

  const std::string content = read_text(svg);
  require(content.find("crest") != std::string::npos, "expected annotation label in svg");
  require(content.find("Signals") != std::string::npos, "expected legend title in svg");
  require(content.find("Intensity") != std::string::npos, "expected colorbar title in svg");
  require(content.find("Q4") != std::string::npos, "expected categorical x label in svg");
  require(content.find("desk") != std::string::npos, "expected subplot axis label in svg");
  require(content.find("font-family=\"IBM Plex Sans\"") != std::string::npos, "expected custom font family in svg");
  require(content.find("fill=\"#f4efe6\"") != std::string::npos, "expected paper theme page background in svg");
  require(content.find("fill=\"#c44536\"") != std::string::npos || content.find("fill=\"#0b3c5d\"") != std::string::npos,
          "expected heatmap cells in svg");
  require(content.find("polygon") != std::string::npos,
          "expected filled area or band geometry in svg");

  return 0;
}
