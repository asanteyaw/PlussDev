#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "pluss/codem/series_factory.h"
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

pluss::Series make_series_1d(const std::vector<double>& values) {
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())});
}

pluss::Series make_series_2d(const std::vector<double>& values, std::int64_t rows, std::int64_t cols) {
  return pluss::make_series(values, {rows, cols});
}

} // namespace

int main() {
  const auto x = make_series_1d({1.0, 2.0, 4.0, 8.0, 16.0});
  const auto y = make_series_1d({2.0, 3.5, 5.0, 7.0, 11.0});
  const auto box_values = make_series_2d({
                                              1.1, 2.0, 2.8,
                                              1.3, 2.1, 3.0,
                                              1.4, 2.4, 3.2,
                                              1.6, 2.6, 3.6,
                                              1.8, 2.8, 3.9,
                                              3.6, 4.2, 5.1,
                                          },
                                          6,
                                          3);
  const auto theta = make_series_1d({0.0, 0.6, 1.1, 1.8, 2.2, 2.9, 3.6, 4.2, 5.1, 5.8});
  const auto rho = make_series_1d({0.8, 1.1, 1.5, 1.2, 0.9, 1.4, 1.7, 1.3, 0.95, 0.75});

  auto fig_semilogx = pluss::plotism::semilogx(x, y, "semilogx");
  auto fig_semilogy = pluss::plotism::semilogy(x, y, "semilogy");
  auto fig_loglog = pluss::plotism::loglog(x, y, "loglog");
  auto fig_stem = pluss::plotism::stem(x, y, "stem");
  auto fig_box = pluss::plotism::boxplot(box_values, "boxplot", {"A", "B", "C"});
  auto fig_violin = pluss::plotism::violin(box_values, "violin", {"A", "B", "C"});
  auto fig_polar = pluss::plotism::polarplot(theta, rho, "polar");

  require(fig_semilogx.subplots().front().x_axis.scale == pluss::plotism::AxisScale::Log, "semilogx should use log x scale");
  require(fig_semilogy.subplots().front().y_axis.scale == pluss::plotism::AxisScale::Log, "semilogy should use log y scale");
  require(fig_loglog.subplots().front().x_axis.scale == pluss::plotism::AxisScale::Log, "loglog should use log x scale");
  require(fig_loglog.subplots().front().y_axis.scale == pluss::plotism::AxisScale::Log, "loglog should use log y scale");
  require(fig_stem.subplots().front().traces.size() >= 2, "stem should emit stem lines plus markers");
  require(!fig_box.subplots().front().traces.empty(), "boxplot should emit traces");
  require(!fig_violin.subplots().front().traces.empty(), "violin should emit traces");
  require(fig_polar.subplots().front().polar_projection, "polarplot should enable polar projection mode");
  require(fig_polar.subplots().front().traces.size() == 1, "polarplot should emit a single polar trace");

  const std::string semilogx_svg = unique_path("plusspl_test_semilogx", ".svg");
  const std::string stem_svg = unique_path("plusspl_test_stem", ".svg");
  const std::string box_svg = unique_path("plusspl_test_boxplot", ".svg");
  const std::string violin_svg = unique_path("plusspl_test_violin", ".svg");
  const std::string polar_svg = unique_path("plusspl_test_polar", ".svg");

  fig_semilogx.save_svg(semilogx_svg, 900, 600);
  fig_stem.save_svg(stem_svg, 900, 600);
  fig_box.save_svg(box_svg, 900, 600);
  fig_violin.save_svg(violin_svg, 900, 600);
  fig_polar.save_svg(polar_svg, 900, 600);

  require(std::filesystem::exists(semilogx_svg) && std::filesystem::file_size(semilogx_svg) > 0,
          "semilogx svg should be generated");
  require(std::filesystem::exists(stem_svg) && std::filesystem::file_size(stem_svg) > 0,
          "stem svg should be generated");
  require(std::filesystem::exists(box_svg) && std::filesystem::file_size(box_svg) > 0,
          "boxplot svg should be generated");
  require(std::filesystem::exists(violin_svg) && std::filesystem::file_size(violin_svg) > 0,
          "violin svg should be generated");
  require(std::filesystem::exists(polar_svg) && std::filesystem::file_size(polar_svg) > 0,
          "polarplot svg should be generated");

  return 0;
}
