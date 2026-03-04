#include <chrono>
#include <cstdint>
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

pluss::Config plot_series_cfg() {
  return pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
}

pluss::Series make_numeric_series(const std::vector<double>& values) {
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())}, plot_series_cfg());
}

pluss::Series make_grid_series(const std::vector<std::vector<double>>& rows) {
  const std::int64_t row_count = static_cast<std::int64_t>(rows.size());
  const std::int64_t col_count = row_count == 0 ? 0 : static_cast<std::int64_t>(rows.front().size());
  std::vector<double> flat;
  flat.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, row_count * col_count)));
  for (const auto& row : rows) {
    flat.insert(flat.end(), row.begin(), row.end());
  }
  return pluss::make_series(flat, {row_count, col_count}, plot_series_cfg());
}

} // namespace

int main() {
  const auto x = make_numeric_series({0.0, 1.0, 2.0, 3.0, 4.0});
  const auto y = make_numeric_series({0.0, 1.0, 2.0, 3.0});
  const auto z = make_grid_series({
      {0.2, 0.4, 0.5, 0.45, 0.3},
      {0.35, 0.55, 0.7, 0.62, 0.5},
      {0.5, 0.7, 0.95, 0.82, 0.66},
      {0.42, 0.61, 0.78, 0.72, 0.58},
  });

  auto waterfall_fig = pluss::plotism::waterfall(x, y, z, "waterfall").with_title("waterfall");
  auto meshz_fig = pluss::plotism::meshz(x, y, z, "meshz").with_title("meshz");
  auto contour_fig = pluss::plotism::contour3(x, y, z, 6, "contour").with_title("contour3");

  pluss::plotism::Style patch_style;
  patch_style.color = "#d97706";
  patch_style.alpha = 0.9;
  auto fill_fig = pluss::plotism::fill3(make_numeric_series({0.4, 2.0, 1.4}),
                                        make_numeric_series({0.5, 0.6, 2.4}),
                                        make_numeric_series({0.3, 0.45, 1.1}),
                                        "fill3",
                                        patch_style)
                      .with_title("fill3");

  const auto waterfall_svg = unique_path("plusspl_test_waterfall", ".svg");
  const auto meshz_svg = unique_path("plusspl_test_meshz", ".svg");
  const auto contour_svg = unique_path("plusspl_test_contour3", ".svg");
  const auto fill_svg = unique_path("plusspl_test_fill3", ".svg");

  waterfall_fig.save_svg(waterfall_svg, 960, 720);
  meshz_fig.save_svg(meshz_svg, 960, 720);
  contour_fig.save_svg(contour_svg, 960, 720);
  fill_fig.save_svg(fill_svg, 960, 720);

  require(std::filesystem::file_size(waterfall_svg) > 0, "waterfall svg should be non-empty");
  require(std::filesystem::file_size(meshz_svg) > 0, "meshz svg should be non-empty");
  require(std::filesystem::file_size(contour_svg) > 0, "contour3 svg should be non-empty");
  require(std::filesystem::file_size(fill_svg) > 0, "fill3 svg should be non-empty");

  const auto waterfall_content = read_text(waterfall_svg);
  const auto meshz_content = read_text(meshz_svg);
  const auto contour_content = read_text(contour_svg);
  const auto fill_content = read_text(fill_svg);

  require(waterfall_content.find("waterfall") != std::string::npos, "expected waterfall title in svg");
  require(meshz_content.find("meshz") != std::string::npos, "expected meshz title in svg");
  require(contour_content.find("contour3") != std::string::npos, "expected contour3 title in svg");
  require(fill_content.find("fill3") != std::string::npos, "expected fill3 title in svg");
  require(fill_content.find("polygon") != std::string::npos, "expected fill3 polygon in svg");

  return 0;
}
