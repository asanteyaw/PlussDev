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

  auto df = pluss::table::DataFrame::load_csv(
      data_file("messy_data.csv").string(),
      /*header=*/"infer",
      /*case=*/"none",
      /*engine=*/"auto",
      /*ncases=*/64);

  auto fig_line = df.plot().line("Agency Code").with_title("Agency Code by case");
  fig_line.show();
  fig_line.save_svg("plot_df_line.svg");
  std::cout << fig_line.to_json() << "\n\n";

  auto fig_scatter = df.plot()
                         .scatter("Agency Code", "Lifecycle Cost")
                         .with_title("Lifecycle Cost vs Agency Code");
  fig_scatter.show();

  auto top_agencies = df.tally("Agency Name", /*top_n=*/8, /*dropna=*/true, /*render=*/false);
  auto ranked_agencies = top_agencies.set_cases("Agency Name", /*drop_feature=*/false);
  auto fig_bar = ranked_agencies.plot().bar("count").with_title("Top Agencies by count");
  fig_bar.show();

  auto fig_hist = df.plot().hist("Lifecycle Cost", /*bins=*/12).with_title("Lifecycle Cost histogram");
  fig_hist.show();
  fig_hist.save_svg("plot_df_hist.svg");

  auto monthly = pluss::table::DataFrame({
      {"sales", pluss::table::Thread(pluss::make_series({120.0, 135.0, 128.0, 150.0, 162.0, 171.0}))},
      {"forecast", pluss::table::Thread(pluss::make_series({118.0, 130.0, 132.0, 147.0, 158.0, 168.0}))},
      {"month", pluss::table::Thread(pluss::make_note({"Jan", "Feb", "Mar", "Apr", "May", "Jun"}))},
  }).set_cases("month", /*drop_feature=*/false);

  auto fig_monthly = monthly.plot().line({"sales", "forecast"}).with_title("Sales vs forecast by month");
  fig_monthly.show();
  fig_monthly.save_svg("plot_df_monthly.svg");

  auto a0 = df.head(20, /*render=*/false);
  auto a1 = df.tail(20, /*render=*/false);
  pluss::table::Atlas atlas({a0, a1});

  auto fig_atlas = atlas.plot(1).line("Agency Code").with_title("Atlas frame(1): Agency Code");
  fig_atlas.show();
  fig_atlas.save_svg("plot_atlas_line.svg");

  // Native 3D plotism usage.
  std::vector<double> z{0.2, 0.4, 0.1, 0.8, 0.6};
  auto fig3 = pluss::plotism::line3(wave_x, wave_y, z, "line3").with_title("PlussPl 3D line");
  fig3.x_label("x").y_label("y").z_label("z");
  fig3.show();
  fig3.save_svg("plot_raw_3d.svg");
  return 0;
#endif
}
