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
  std::vector<double> z;
  x.reserve(96);
  y.reserve(96);
  z.reserve(96);
  for (std::size_t i = 0; i < 96; ++i) {
    const double t = static_cast<double>(i) * 0.1;
    x.push_back(std::cos(t));
    y.push_back(std::sin(t));
    z.push_back(0.05 * static_cast<double>(i));
  }

  auto fig = pluss::plotism::line3(x, y, z, "helix").with_title("test3d");
  fig.x_label("x").y_label("y").z_label("z");

  const std::string svg = unique_path("plusspl_test_3d", ".svg");
  const std::string png = unique_path("plusspl_test_3d", ".png");
  const std::string pdf = unique_path("plusspl_test_3d", ".pdf");

  fig.save_svg(svg, 900, 700);
  fig.save_png(png, 900, 700);
  fig.save_pdf(pdf, 900, 700);

  require(std::filesystem::exists(svg), "3D svg file was not created");
  require(std::filesystem::exists(png), "3D png file was not created");
  require(std::filesystem::exists(pdf), "3D pdf file was not created");
  require(std::filesystem::file_size(svg) > 0, "3D svg file is empty");
  require(std::filesystem::file_size(png) > 0, "3D png file is empty");
  require(std::filesystem::file_size(pdf) > 0, "3D pdf file is empty");
  require_png_signature(png);

  const bool opened = fig.show_window(false);
  (void)opened;
  fig.show();

  std::cout << "[PASS] 3D export/show test\n";
  return 0;
}
