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
  fig.with_title("rich2d")
      .title("Styled 2D subplot")
      .x_label("quarter")
      .y_label("value")
      .legend(pluss::plotism::Legend{true, pluss::plotism::LegendLocation::TopLeft, "Signals"});

  pluss::plotism::Style dashed;
  dashed.color = "#0f766e";
  dashed.line_style = "dashdot";
  dashed.marker = "diamond";
  dashed.line_width = 2.2;

  pluss::plotism::Style scatter_style;
  scatter_style.color = "#c2410c";
  scatter_style.marker = "square";

  fig.add_line(x, y1, "series-a", {"Q1", "Q2", "Q3", "Q4", "Q5", "Q6", "Q7", "Q8"}, dashed);
  fig.add_scatter(x, y2, "series-b", {}, scatter_style);

  const std::string svg = unique_path("plusspl_test_rich_2d", ".svg");
  fig.save_svg(svg, 980, 640);

  const std::string content = read_text(svg);
  require(content.find("stroke-dasharray") != std::string::npos, "expected dashed line styling in svg");
  require(content.find("Signals") != std::string::npos, "expected legend title in svg");
  require(content.find("Q4") != std::string::npos, "expected categorical x label in svg");
  require(content.find("polygon") != std::string::npos || content.find("<rect") != std::string::npos,
          "expected marker geometry in svg");

  return 0;
}
