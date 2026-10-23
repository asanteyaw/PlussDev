#include <pluss/pluss.h>

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

std::filesystem::path data_file(const std::string& name) {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "data" / name;
}

std::filesystem::path output_dir() {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "examples" / "outputs" / "dataframe_plot_demo";
}

} // namespace

int main() {
#if !__has_include("pluss/pl.h")
  std::cout << "pluss/pl.h is unavailable in this build.\n";
  return 0;
#else
  std::filesystem::create_directories(output_dir());
  std::filesystem::current_path(output_dir());

  std::vector<double> wave_x{0.0, 1.0, 2.0, 3.0, 4.0};
  std::vector<double> wave_y{1.0, 2.0, 1.5, 3.0, 2.2};

  auto raw_fig = pluss::pl::studio("raw-2d");
  raw_fig.headline("PlussPl raw line");
  auto& raw_pane = raw_fig.pane2();
  raw_pane.trend(wave_x, wave_y).label("raw-2d");
  raw_pane.x_label("x").y_label("y");
  raw_fig.save_svg("plot_raw_wave.svg");
  raw_fig.save_pdf("plot_raw_wave.pdf");

  auto df = pluss::table::DataFrame::load_csv(
      data_file("messy_data.csv").string(),
      /*header=*/"infer",
      /*case=*/"none",
      /*engine=*/"auto",
      /*ncases=*/64);

  auto fig_line = df.plot().line("Agency Code");
  fig_line.headline("Agency Code by case");
  fig_line.save_svg("plot_df_line.svg");
  fig_line.save_pdf("plot_df_line.pdf");

  auto fig_scatter = df.plot().scatter("Agency Code", "Lifecycle Cost");
  fig_scatter.headline("Lifecycle Cost vs Agency Code");
  fig_scatter.save_pdf("plot_df_scatter.pdf");

  auto top_agencies = df.features({"Agency Name"}).freq({
      .top = 8,
      .dropnull = true,
  });
  auto ranked_agencies = pluss::table::DataFrame({{"count", top_agencies}})
                             .labels().set(top_agencies.label().values());
  auto fig_bar = ranked_agencies.plot().bar("count");
  fig_bar.headline("Top Agencies by count");
  fig_bar.save_pdf("plot_df_bar.pdf");

  auto fig_hist = df.plot().hist("Lifecycle Cost", /*bins=*/12);
  fig_hist.headline("Lifecycle Cost histogram");
  fig_hist.save_svg("plot_df_hist.svg");
  fig_hist.save_pdf("plot_df_hist.pdf");

  auto monthly = pluss::table::DataFrame({
      {"sales", pluss::table::Slate(pluss::make_series({120.0, 135.0, 128.0, 150.0, 162.0, 171.0}))},
      {"forecast", pluss::table::Slate(pluss::make_series({118.0, 130.0, 132.0, 147.0, 158.0, 168.0}))},
      {"month", pluss::table::Slate(pluss::make_note({"Jan", "Feb", "Mar", "Apr", "May", "Jun"}))},
  }).labels().set("month", /*drop_feature=*/false);

  auto fig_monthly = monthly.plot().line({"sales", "forecast"});
  fig_monthly.headline("Sales vs forecast by month");
  fig_monthly.save_svg("plot_df_monthly.svg");
  fig_monthly.save_pdf("plot_df_monthly.pdf");

  pluss::table::DataFramePlotOptions monthly_opts;
  monthly_opts.default_x_feature = "month";
  monthly_opts.use_tight_layout = true;
  monthly_opts.tight_layout_padding = 4.0;
  monthly_opts.width = 1280.0;
  monthly_opts.height = 760.0;
  auto fig_monthly_cfg = monthly.plot(monthly_opts).line({"sales", "forecast"});
  fig_monthly_cfg.headline("Sales vs forecast (plot options)");
  fig_monthly_cfg.save_svg("plot_df_monthly_options.svg");
  fig_monthly_cfg.save_pdf("plot_df_monthly_options.pdf");

  auto fig_step = monthly.plot().step("sales");
  fig_step.headline("Monthly sales step plot");
  fig_step.save_svg("plot_df_step.svg");
  fig_step.save_pdf("plot_df_step.pdf");

  auto fig_area = monthly.plot().area({"sales", "forecast"});
  fig_area.headline("Monthly sales area plot");
  fig_area.save_svg("plot_df_area.svg");
  fig_area.save_pdf("plot_df_area.pdf");

  auto fig_area_stacked = monthly.plot().area_stacked({"sales", "forecast"});
  fig_area_stacked.headline("Monthly stacked area");
  fig_area_stacked.save_pdf("plot_df_area_stacked.pdf");

  auto fig_grouped_bar = monthly.plot().bar({"sales", "forecast"});
  fig_grouped_bar.headline("Monthly grouped bars");
  fig_grouped_bar.save_svg("plot_df_grouped_bar.svg");
  fig_grouped_bar.save_pdf("plot_df_grouped_bar.pdf");

  auto fig_stacked_bar = monthly.plot().bar_stacked({"sales", "forecast"});
  fig_stacked_bar.headline("Monthly stacked bars");
  fig_stacked_bar.save_pdf("plot_df_stacked_bar.pdf");

  auto fig_heatmap = monthly.plot().heatmap({"sales", "forecast"});
  fig_heatmap.headline("Monthly feature heatmap");
  fig_heatmap.save_svg("plot_df_heatmap.svg");
  fig_heatmap.save_pdf("plot_df_heatmap.pdf");

  auto fig_corr = monthly.plot().corr_heatmap({"sales", "forecast"});
  fig_corr.headline("Monthly correlation heatmap");
  fig_corr.save_svg("plot_df_corr_heatmap.svg");
  fig_corr.save_pdf("plot_df_corr_heatmap.pdf");

  auto fig_auto = monthly.plot().auto_plot({"sales", "forecast"});
  fig_auto.headline("Monthly auto plot");
  fig_auto.save_pdf("plot_df_auto.pdf");

  auto metrics = pluss::table::DataFrame({
      {"ret", pluss::table::Slate(pluss::make_series({0.11, 0.15, 0.09, 0.13, 0.17, 0.12}))},
      {"vol", pluss::table::Slate(pluss::make_series({0.21, 0.26, 0.18, 0.24, 0.28, 0.2}))},
      {"carry", pluss::table::Slate(pluss::make_series({0.03, 0.04, 0.025, 0.038, 0.045, 0.031}))},
  });

  auto fig_matrix = metrics.plot().scatter_matrix();
  fig_matrix.headline("Scatter matrix");
  fig_matrix.save_pdf("plot_df_scatter_matrix.pdf");

  auto fig_box = metrics.plot().box();
  fig_box.headline("Metric box plot");
  fig_box.save_pdf("plot_df_box.pdf");

  auto regime_points = pluss::table::DataFrame({
      {"ret", pluss::table::Slate(pluss::make_series({0.08, 0.11, 0.12, 0.1, 0.14, 0.16, 0.07, 0.09}))},
      {"vol", pluss::table::Slate(pluss::make_series({0.18, 0.23, 0.27, 0.2, 0.29, 0.31, 0.16, 0.19}))},
      {"desk", pluss::table::Slate(pluss::make_note({"macro", "macro", "rates", "rates", "equity", "equity", "credit", "credit"}))},
  });

  pluss::table::DataFramePlotOptions hue_opts;
  hue_opts.default_hue_feature = "desk";
  hue_opts.use_tight_layout = true;
  auto fig_scatter_hue = regime_points.plot(hue_opts).scatter("ret", "vol");
  fig_scatter_hue.headline("Scatter with hue");
  fig_scatter_hue.save_pdf("plot_df_scatter_hue.pdf");

  auto option_surface = pluss::table::DataFrame({
      {"maturity", pluss::table::Slate(pluss::make_note({"1M", "1M", "1M", "3M", "3M", "3M", "6M", "6M", "6M"}))},
      {"delta_bucket", pluss::table::Slate(pluss::make_note({"25P", "ATM", "25C", "25P", "ATM", "25C", "25P", "ATM", "25C"}))},
      {"implied_vol", pluss::table::Slate(pluss::make_series({0.29, 0.24, 0.22, 0.31, 0.26, 0.24, 0.34, 0.29, 0.27}))},
  });

  auto fig_surface_heatmap = option_surface.plot().heatmap("maturity", "delta_bucket", "implied_vol");
  fig_surface_heatmap.headline("Implied vol surface heatmap");
  fig_surface_heatmap.save_svg("plot_df_surface_heatmap.svg");
  fig_surface_heatmap.save_pdf("plot_df_surface_heatmap.pdf");

  std::vector<double> z{0.2, 0.4, 0.1, 0.8, 0.6};
  auto fig3 = pluss::pl::studio("line3");
  fig3.headline("PlussPl 3D line");
  auto& pane3 = fig3.pane3();
  pane3.path3(wave_x, wave_y, z).label("line3");
  pane3.x_label("x").y_label("y").z_label("z");
  fig3.save_svg("plot_raw_3d.svg");
  fig3.save_pdf("plot_raw_3d.pdf");
  return 0;
#endif
}
