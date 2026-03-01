#include <chrono>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
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

void require_png_signature(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  std::vector<unsigned char> header(8, 0u);
  in.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
  require(in.gcount() == static_cast<std::streamsize>(header.size()), "PNG header was not readable");
  const std::vector<unsigned char> expected{0x89u, 'P', 'N', 'G', '\r', '\n', 0x1au, '\n'};
  require(header == expected, "PNG file does not have a valid signature");
}

} // namespace

int main() {
  std::vector<double> x;
  std::vector<double> y;
  x.reserve(128);
  y.reserve(128);
  for (std::size_t i = 0; i < 128; ++i) {
    const double xv = static_cast<double>(i) * 0.05;
    x.push_back(xv);
    y.push_back(std::sin(xv));
  }

  auto fig = pluss::plotism::line(x, y, "wave").with_title("test2d");
  fig.x_label("x").y_label("y");

  const std::string svg = unique_path("plusspl_test_2d", ".svg");
  const std::string png = unique_path("plusspl_test_2d", ".png");
  const std::string pdf = unique_path("plusspl_test_2d", ".pdf");

  fig.save_svg(svg, 900, 600);
  fig.save_png(png, 900, 600);
  fig.save_pdf(pdf, 900, 600);

  require(std::filesystem::exists(svg), "2D svg file was not created");
  require(std::filesystem::exists(png), "2D png file was not created");
  require(std::filesystem::exists(pdf), "2D pdf file was not created");
  require(std::filesystem::file_size(svg) > 0, "2D svg file is empty");
  require(std::filesystem::file_size(png) > 0, "2D png file is empty");
  require(std::filesystem::file_size(pdf) > 0, "2D pdf file is empty");
  require_png_signature(png);

  const bool opened = fig.show_window(false);
  (void)opened;
  fig.show();

  std::cout << "[PASS] 2D export/show test\n";
  return 0;
}
