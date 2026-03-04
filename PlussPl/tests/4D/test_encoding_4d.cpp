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

} // namespace

int main() {
  std::vector<double> x;
  std::vector<double> y;
  std::vector<double> z;
  std::vector<double> w;
  for (std::size_t j = 0; j < 8; ++j) {
    const double expiry = 0.2 + static_cast<double>(j) * 0.22;
    for (std::size_t i = 0; i < 10; ++i) {
      const double strike = 80.0 + static_cast<double>(i) * 4.0;
      const double skew = (strike - 100.0) / 24.0;
      x.push_back(strike);
      y.push_back(expiry);
      z.push_back(0.18 + 0.08 * skew * skew + 0.025 * expiry);
      w.push_back(0.4 + 0.35 * skew + 0.2 * expiry);
    }
  }

  auto fig4 = pluss::plotism::scatter4(make_numeric_series(x), make_numeric_series(y), make_numeric_series(z), make_numeric_series(w), "surface cloud")
                  .with_title("4d")
                  .x_label("strike")
                  .y_label("expiry")
                  .z_label("vol")
                  .w_label("regime")
                  .theme("slate")
                  .font_family("IBM Plex Sans")
                  .encode_color("regime", 5);

  auto color_fig = fig4.realize_color_figure();
  const auto svg = unique_path("plusspl_test_4d_color", ".svg");
  color_fig.save_svg(svg, 960, 720);
  const auto content = read_text(svg);
  require(content.find("regime") != std::string::npos, "expected 4d legend/colorbar title in svg");
  require(content.find("font-family=\"IBM Plex Sans\"") != std::string::npos, "expected custom font in 4d svg");

  auto size_fig = fig4.encode_size("regime", 5).realize_size_figure();
  const auto size_svg = unique_path("plusspl_test_4d_size", ".svg");
  size_fig.save_svg(size_svg, 960, 720);
  const auto size_content = read_text(size_svg);
  require(size_content.find("surface cloud") != std::string::npos, "expected 4d size svg to include trace name");

  auto slices = fig4.encode_slice("regime", 4).realize_slice_figures();
  require(!slices.empty(), "expected at least one 4d slice figure");
  const auto slice_pdf = unique_path("plusspl_test_4d_slice", ".pdf");
  slices.front().save_pdf(slice_pdf, 900, 680);
  require(std::filesystem::exists(slice_pdf), "expected 4d slice pdf to exist");
  require(std::filesystem::file_size(slice_pdf) > 0, "expected 4d slice pdf to be non-empty");

  auto frames = fig4.encode_frame("regime", 4).realize_frame_figures();
  require(!frames.empty(), "expected at least one 4d frame figure");
  auto frame_view = fig4.encode_frame("regime", 4).realize_frame_view();
  require(frame_view.panel_count() == frames.size(), "expected frame view to preserve frame bin count");
  const auto frame_svg = unique_path("plusspl_test_4d_frame_view", ".svg");
  frame_view.save_svg(frame_svg, 1280, 920);
  const auto frame_content = read_text(frame_svg);
  require(frame_content.find("regime [") != std::string::npos, "expected frame view svg to include panel titles");

  auto facets = fig4.encode_facet("regime", 4).realize_facet_figures();
  require(!facets.empty(), "expected at least one 4d facet figure");
  auto facet_view = fig4.encode_facet("regime", 4).realize_facet_view();
  require(facet_view.panel_count() == facets.size(), "expected facet view to preserve facet bin count");
  const auto facet_pdf = unique_path("plusspl_test_4d_facet_view", ".pdf");
  facet_view.save_pdf(facet_pdf, 1280, 920);
  require(std::filesystem::exists(facet_pdf), "expected 4d facet view pdf to exist");
  require(std::filesystem::file_size(facet_pdf) > 0, "expected 4d facet view pdf to be non-empty");

  return 0;
}
