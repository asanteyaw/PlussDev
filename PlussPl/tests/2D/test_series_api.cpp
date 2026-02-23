#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>

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

} // namespace

int main() {
  const auto y = pluss::rand({50});
  const auto x = pluss::linspace(0.0, 1.0, 50, pluss::ScalarType::Float64);
  const auto h = pluss::randn({400}, pluss::ScalarType::Float64, 11);

  auto fig = pluss::plotism::line(y, "rand(50)").with_title("series-api");
  fig.add_scatter(x, y, "scatter(x,y)");
  fig.add_bar(y, "bar(y)");
  fig.add_hist(h, 16, "hist(randn)");

  const auto& subplots = fig.subplots();
  require(subplots.size() == 1, "expected a single subplot");
  require(subplots.front().traces.size() == 4, "expected 4 traces");
  require(subplots.front().traces[0].x.size() == 50, "line(y): x should auto-index to size 50");
  require(subplots.front().traces[0].y.size() == 50, "line(y): y should have size 50");
  require(subplots.front().traces[1].x.size() == 50, "scatter(x,y): x should have size 50");
  require(subplots.front().traces[2].y.size() == 50, "bar(y): y should have size 50");
  require(subplots.front().traces[3].bins == 16, "hist: bins should be 16");

  const std::string svg = unique_path("plusspl_test_series_api", ".svg");
  const std::string pdf = unique_path("plusspl_test_series_api", ".pdf");
  fig.save_svg(svg, 900, 600);
  fig.save_pdf(pdf, 900, 600);

  require(std::filesystem::exists(svg), "series-api svg file was not created");
  require(std::filesystem::exists(pdf), "series-api pdf file was not created");
  require(std::filesystem::file_size(svg) > 0, "series-api svg file is empty");
  require(std::filesystem::file_size(pdf) > 0, "series-api pdf file is empty");

  return 0;
}
