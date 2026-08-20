#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::make_note;
using pluss::make_series;
using pluss::make_timeline;
using pluss::pl::Studio;
using pluss::pl::Theme;
using pluss::pl::flash::BarPlotOptions;
using pluss::pl::flash::TableColumns;
using pluss::table::DataFrame;

const std::filesystem::path kOutDir =
    std::filesystem::temp_directory_path() / "pluss_flash_frame_plot_demo";

void save_svg(Studio& fig, std::string_view stem) {
  std::filesystem::create_directories(kOutDir);
  const auto path = kOutDir / (std::string(stem) + ".svg");
  fig.save_svg(path.string());
  std::cout << "saved " << path << "\n";
}

DataFrame curve_exact_two() {
  return DataFrame({
      {"observed_at", make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
          "2026-01-04T00:00:00Z",
          "2026-01-05T00:00:00Z",
          "2026-01-06T00:00:00Z",
      })},
      {"sales", make_series(std::vector<double>{10.0, 12.0, 11.0, 13.5, 14.0, 12.8})},
  }).title("curve_exact_two");
}

DataFrame curve_explicit_many() {
  return DataFrame({
      {"observed_at", make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
          "2026-01-04T00:00:00Z",
          "2026-01-05T00:00:00Z",
          "2026-01-06T00:00:00Z",
      })},
      {"sales", make_series(std::vector<double>{10.0, 12.0, 11.0, 13.5, 14.0, 12.8})},
      {"margin", make_series(std::vector<double>{1.5, 1.8, 1.4, 2.0, 2.2, 1.9})},
      {"visits", make_series(std::vector<double>{80.0, 90.0, 85.0, 97.0, 102.0, 95.0})},
  }).title("curve_explicit_many");
}

DataFrame numeric_one() {
  return DataFrame({
      {"score", make_series(std::vector<double>{
          91.0, 84.0, 88.0, 73.0, 95.0, 79.0, 82.0, 90.0, 86.0, 77.0,
      })},
  }).title("numeric_one");
}

DataFrame numeric_many() {
  return DataFrame({
      {"score", make_series(std::vector<double>{
          91.0, 84.0, 88.0, 73.0, 95.0, 79.0, 82.0, 90.0, 86.0, 77.0,
      })},
      {"bonus", make_series(std::vector<double>{
          1.0, 0.0, 2.0, 1.0, 3.0, 0.0, 1.0, 2.0, 1.0, 0.0,
      })},
  }).title("numeric_many");
}

DataFrame text_one() {
  return DataFrame({
      {"city", make_note({
          "Accra", "Tema", "Accra", "Kumasi", "Tema", "Accra", "Tamale", "Kumasi",
      })},
  }).title("text_one");
}

DataFrame bars_exact_two() {
  return DataFrame({
      {"quarter", make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0})},
      {"revenue", make_series(std::vector<double>{18.0, 21.0, 19.5, 24.0})},
  }).title("bars_exact_two");
}

DataFrame bars_explicit_many() {
  return DataFrame({
      {"quarter", make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0})},
      {"revenue", make_series(std::vector<double>{18.0, 21.0, 19.5, 24.0})},
      {"expense", make_series(std::vector<double>{12.0, 13.0, 12.8, 14.5})},
  }).title("bars_explicit_many");
}

DataFrame space_exact_three() {
  return DataFrame({
      {"x", make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0, 5.0})},
      {"y", make_series(std::vector<double>{0.0, 1.0, 0.5, 1.5, 1.0, 2.0})},
      {"z", make_series(std::vector<double>{1.0, 1.3, 1.6, 2.0, 2.3, 2.6})},
  }).title("space_exact_three");
}

DataFrame space_explicit_many() {
  return DataFrame({
      {"x", make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0, 5.0})},
      {"y", make_series(std::vector<double>{0.0, 1.0, 0.5, 1.5, 1.0, 2.0})},
      {"z", make_series(std::vector<double>{1.0, 1.3, 1.6, 2.0, 2.3, 2.6})},
      {"w", make_series(std::vector<double>{5.0, 4.5, 4.0, 3.5, 3.0, 2.5})},
  }).title("space_explicit_many");
}

void demo_lineplot() {
  Studio fig("lineplot_from_frame", Theme::paper());
  auto& pane = fig.pane2("lineplot");
  pluss::pl::flash::lineplot(pane, curve_exact_two()).label("auto two-column");
  save_svg(fig, "lineplot_from_frame");
}

void demo_scatterplot() {
  Studio fig("scatterplot_from_frame", Theme::paper());
  auto& pane = fig.pane2("scatterplot");
  pluss::pl::flash::scatterplot(
      pane,
      curve_explicit_many(),
      TableColumns{.x_feature = "visits", .y_feature = "margin"}).label("explicit columns");
  save_svg(fig, "scatterplot_from_frame");
}

void demo_stepplot() {
  Studio fig("stepplot_from_frame", Theme::paper());
  auto& pane = fig.pane2("stepplot");
  pluss::pl::flash::stepplot(pane, curve_exact_two()).label("auto two-column");
  save_svg(fig, "stepplot_from_frame");
}

void demo_areaplot() {
  Studio fig("areaplot_from_frame", Theme::paper());
  auto& pane = fig.pane2("areaplot");
  pluss::pl::flash::areaplot(
      pane,
      curve_explicit_many(),
      TableColumns{.x_feature = "observed_at", .y_feature = "sales"},
      -0.2).label("explicit columns");
  save_svg(fig, "areaplot_from_frame");
}

void demo_histplot() {
  Studio fig("histplot_from_frame", Theme::paper());
  auto& pane = fig.pane2("histplot");
  pluss::pl::flash::histplot(
      pane,
      numeric_many(),
      TableColumns{.feature = "bonus"},
      6).label("explicit feature");
  save_svg(fig, "histplot_from_frame");
}

void demo_displot() {
  Studio fig("displot_from_frame", Theme::paper());
  auto& pane = fig.pane2("displot");
  pluss::pl::flash::displot(pane, numeric_one(), {}, 8).label("single column auto");
  save_svg(fig, "displot_from_frame");
}

void demo_kdeplot() {
  Studio fig("kdeplot_from_frame", Theme::paper());
  auto& pane = fig.pane2("kdeplot");
  pluss::pl::flash::kdeplot(pane, numeric_one(), {}, 180).label("single column auto");
  save_svg(fig, "kdeplot_from_frame");
}

void demo_ecdfplot() {
  Studio fig("ecdfplot_from_frame", Theme::paper());
  auto& pane = fig.pane2("ecdfplot");
  pluss::pl::flash::ecdfplot(
      pane,
      numeric_many(),
      TableColumns{.feature = "score"}).label("explicit feature");
  save_svg(fig, "ecdfplot_from_frame");
}

void demo_rugplot() {
  Studio fig("rugplot_from_frame", Theme::paper());
  auto& pane = fig.pane2("rugplot");
  pluss::pl::flash::rugplot(
      pane,
      numeric_many(),
      TableColumns{.feature = "score"},
      0.09).label("explicit feature");
  save_svg(fig, "rugplot_from_frame");
}

void demo_countplot() {
  Studio fig("countplot_from_frame", Theme::paper());
  auto& pane = fig.pane2("countplot");
  pluss::pl::flash::countplot(pane, text_one()).label("single text column auto");
  save_svg(fig, "countplot_from_frame");
}

void demo_barplot() {
  Studio fig("barplot_from_frame", Theme::paper());
  auto& pane = fig.pane2("barplot");
  pluss::pl::flash::barplot(
      pane,
      bars_explicit_many(),
      TableColumns{.x_feature = "quarter", .y_feature = "revenue"},
      BarPlotOptions{}).label("explicit columns");
  save_svg(fig, "barplot_from_frame");
}

void demo_barplot_horizontal_univariate() {
  Studio fig("barplot_horizontal_from_frame", Theme::paper());
  auto& pane = fig.pane2("barplot_horizontal");
  pluss::pl::flash::barplot(
      pane,
      numeric_one(),
      TableColumns{.feature = "score"},
      BarPlotOptions{.horizontal = true}).label("single numeric column");
  save_svg(fig, "barplot_horizontal_from_frame");
}

void demo_path3() {
  Studio fig("path3_from_frame", Theme::paper());
  auto& pane = fig.pane3("path3");
  pluss::pl::flash::path3(pane, space_exact_three()).label("auto three-column");
  save_svg(fig, "path3_from_frame");
}

void demo_cloud3() {
  Studio fig("cloud3_from_frame", Theme::paper());
  auto& pane = fig.pane3("cloud3");
  pluss::pl::flash::cloud3(
      pane,
      space_explicit_many(),
      TableColumns{.x_feature = "x", .y_feature = "z", .z_feature = "w"}).label("explicit columns");
  save_svg(fig, "cloud3_from_frame");
}

void demo_plot3d() {
  Studio fig("plot3d_from_frame", Theme::paper());
  auto& pane = fig.pane3("plot3D");
  pluss::pl::flash::plot3D(pane, space_exact_three()).label("auto three-column");
  save_svg(fig, "plot3d_from_frame");
}

void demo_scatter3d() {
  Studio fig("scatter3d_from_frame", Theme::paper());
  auto& pane = fig.pane3("scatter3D");
  pluss::pl::flash::scatter3D(
      pane,
      space_explicit_many(),
      TableColumns{.x_feature = "x", .y_feature = "y", .z_feature = "z"}).label("explicit columns");
  save_svg(fig, "scatter3d_from_frame");
}

void demo_jointplot() {
  Studio fig("jointplot_from_frame", Theme::paper());
  pluss::pl::flash::jointplot(
      fig,
      curve_explicit_many(),
      TableColumns{.x_feature = "visits", .y_feature = "sales"},
      "reg");
  save_svg(fig, "jointplot_from_frame");
}

} // namespace

int main() {
  demo_lineplot();
  demo_scatterplot();
  demo_stepplot();
  demo_areaplot();
  demo_histplot();
  demo_displot();
  demo_kdeplot();
  demo_ecdfplot();
  demo_rugplot();
  demo_countplot();
  demo_barplot();
  demo_barplot_horizontal_univariate();
  demo_path3();
  demo_cloud3();
  demo_plot3d();
  demo_scatter3d();
  demo_jointplot();
  std::cout << "output_dir=" << kOutDir << "\n";
  return 0;
}
