#include <pluss/pluss.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::filesystem::path data_file(const std::string& name) {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "data" / name;
}

} // namespace

int main() {
#if !__has_include("pluss/pl.h")
  std::cout << "pluss/pl.h is unavailable in this build.\n";
  return 0;
#else
  // PlussPl can be used directly without DataFrame.
  std::vector<double> wave_x{0.0, 1.0, 2.0, 3.0, 4.0};
  std::vector<double> wave_y{1.0, 2.0, 1.5, 3.0, 2.2};
  auto raw_fig = pluss::plotism::line(wave_x, wave_y, "raw-2d").with_title("PlussPl raw line");
  raw_fig.x_label("x").y_label("y");
  raw_fig.show();
  raw_fig.save_svg("plot_raw_wave.svg");
  raw_fig.save_pdf("plot_raw_wave.pdf");

  auto df = pluss::table::DataFrame::load_csv(
      data_file("messy_data.csv").string(),
      /*header=*/"infer",
      /*case=*/"none",
      /*engine=*/"auto",
      /*ncases=*/64);

  auto fig_line = df.plot().line("Agency Code").with_title("Agency Code by case");
  fig_line.show();
  fig_line.save_svg("plot_df_line.svg");
  fig_line.save_pdf("plot_df_line.pdf");
  std::cout << fig_line.to_json() << "\n\n";

  auto fig_scatter = df.plot()
                         .scatter("Agency Code", "Lifecycle Cost")
                         .with_title("Lifecycle Cost vs Agency Code");
  fig_scatter.show();
  fig_scatter.save_pdf("plot_df_scatter.pdf");

  auto top_agencies = df.tally("Agency Name", /*top_n=*/8, /*dropna=*/true, /*render=*/false);
  auto ranked_agencies = top_agencies.set_cases("Agency Name", /*drop_feature=*/false);
  auto fig_bar = ranked_agencies.plot().bar("count").with_title("Top Agencies by count");
  fig_bar.show();
  fig_bar.save_pdf("plot_df_bar.pdf");

  auto fig_hist = df.plot().hist("Lifecycle Cost", /*bins=*/12).with_title("Lifecycle Cost histogram");
  fig_hist.show();
  fig_hist.save_svg("plot_df_hist.svg");
  fig_hist.save_pdf("plot_df_hist.pdf");

  auto monthly = pluss::table::DataFrame({
      {"sales", pluss::table::Thread(pluss::make_series({120.0, 135.0, 128.0, 150.0, 162.0, 171.0}))},
      {"forecast", pluss::table::Thread(pluss::make_series({118.0, 130.0, 132.0, 147.0, 158.0, 168.0}))},
      {"month", pluss::table::Thread(pluss::make_note({"Jan", "Feb", "Mar", "Apr", "May", "Jun"}))},
  }).set_cases("month", /*drop_feature=*/false);

  auto fig_monthly = monthly.plot().line({"sales", "forecast"}).with_title("Sales vs forecast by month");
  fig_monthly.show();
  fig_monthly.save_svg("plot_df_monthly.svg");
  fig_monthly.save_pdf("plot_df_monthly.pdf");

  auto fig_step = monthly.plot().step("sales").with_title("Monthly sales step plot");
  fig_step.show();
  fig_step.save_svg("plot_df_step.svg");
  fig_step.save_pdf("plot_df_step.pdf");

  auto fig_area = monthly.plot().area({"sales", "forecast"}).with_title("Monthly sales area plot");
  fig_area.show();
  fig_area.save_svg("plot_df_area.svg");
  fig_area.save_pdf("plot_df_area.pdf");

  auto fig_area_stacked = monthly.plot().area_stacked({"sales", "forecast"}).with_title("Monthly stacked area");
  fig_area_stacked.show();
  fig_area_stacked.save_pdf("plot_df_area_stacked.pdf");

  auto fig_grouped_bar = monthly.plot().bar({"sales", "forecast"}).with_title("Monthly grouped bars");
  fig_grouped_bar.show();
  fig_grouped_bar.save_svg("plot_df_grouped_bar.svg");
  fig_grouped_bar.save_pdf("plot_df_grouped_bar.pdf");

  auto fig_stacked_bar = monthly.plot().bar_stacked({"sales", "forecast"}).with_title("Monthly stacked bars");
  fig_stacked_bar.show();
  fig_stacked_bar.save_pdf("plot_df_stacked_bar.pdf");

  auto fig_heatmap = monthly.plot().heatmap({"sales", "forecast"}).with_title("Monthly feature heatmap");
  fig_heatmap.show();
  fig_heatmap.save_svg("plot_df_heatmap.svg");
  fig_heatmap.save_pdf("plot_df_heatmap.pdf");

  auto fig_corr = monthly.plot().corr_heatmap({"sales", "forecast"}).with_title("Monthly correlation heatmap");
  fig_corr.show();
  fig_corr.save_svg("plot_df_corr_heatmap.svg");
  fig_corr.save_pdf("plot_df_corr_heatmap.pdf");

  auto fig_auto = monthly.plot().auto_plot({"sales", "forecast"}).with_title("Monthly auto plot");
  fig_auto.show();
  fig_auto.save_pdf("plot_df_auto.pdf");

  auto metrics = pluss::table::DataFrame({
      {"ret", pluss::table::Thread(pluss::make_series({0.11, 0.15, 0.09, 0.13, 0.17, 0.12}))},
      {"vol", pluss::table::Thread(pluss::make_series({0.21, 0.26, 0.18, 0.24, 0.28, 0.2}))},
      {"carry", pluss::table::Thread(pluss::make_series({0.03, 0.04, 0.025, 0.038, 0.045, 0.031}))},
  });

  auto fig_matrix = metrics.plot().scatter_matrix().with_title("Scatter matrix");
  fig_matrix.show();
  fig_matrix.save_pdf("plot_df_scatter_matrix.pdf");

  auto fig_box = metrics.plot().box().with_title("Metric box plot");
  fig_box.show();
  fig_box.save_pdf("plot_df_box.pdf");

  auto regime_points = pluss::table::DataFrame({
      {"ret", pluss::table::Thread(pluss::make_series({0.08, 0.11, 0.12, 0.1, 0.14, 0.16, 0.07, 0.09}))},
      {"vol", pluss::table::Thread(pluss::make_series({0.18, 0.23, 0.27, 0.2, 0.29, 0.31, 0.16, 0.19}))},
      {"desk", pluss::table::Thread(pluss::make_note({"macro", "macro", "rates", "rates", "equity", "equity", "credit", "credit"}))},
  });

  auto fig_scatter_hue = regime_points.plot().scatter("ret", "vol", "desk").with_title("Scatter with hue");
  fig_scatter_hue.show();
  fig_scatter_hue.save_pdf("plot_df_scatter_hue.pdf");

  auto option_surface = pluss::table::DataFrame({
      {"maturity", pluss::table::Thread(pluss::make_note({"1M", "1M", "1M", "3M", "3M", "3M", "6M", "6M", "6M"}))},
      {"delta_bucket", pluss::table::Thread(pluss::make_note({"25P", "ATM", "25C", "25P", "ATM", "25C", "25P", "ATM", "25C"}))},
      {"implied_vol", pluss::table::Thread(pluss::make_series({0.29, 0.24, 0.22, 0.31, 0.26, 0.24, 0.34, 0.29, 0.27}))},
  });

  auto fig_surface_heatmap =
      option_surface.plot().heatmap("maturity", "delta_bucket", "implied_vol").with_title("Implied vol surface heatmap");
  fig_surface_heatmap.show();
  fig_surface_heatmap.save_svg("plot_df_surface_heatmap.svg");
  fig_surface_heatmap.save_pdf("plot_df_surface_heatmap.pdf");

  auto a0 = df.head(20, /*render=*/false);
  auto a1 = df.tail(20, /*render=*/false);
  pluss::table::Atlas atlas({a0, a1});

  auto fig_atlas = atlas.plot(1).line("Agency Code").with_title("Atlas frame(1): Agency Code");
  fig_atlas.show();
  fig_atlas.save_svg("plot_atlas_line.svg");
  fig_atlas.save_pdf("plot_atlas_line.pdf");

  // Native 3D plotism usage.
  std::vector<double> z{0.2, 0.4, 0.1, 0.8, 0.6};
  auto fig3 = pluss::plotism::line3(wave_x, wave_y, z, "line3").with_title("PlussPl 3D line");
  fig3.x_label("x").y_label("y").z_label("z");
  fig3.show();
  fig3.save_svg("plot_raw_3d.svg");
  fig3.save_pdf("plot_raw_3d.pdf");
  return 0;
#endif
}
