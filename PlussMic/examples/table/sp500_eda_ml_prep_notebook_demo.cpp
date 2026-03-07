#include <pluss/pluss.h>

#include <cstdint>
#include <filesystem>
#include <iostream>

int main() {
  namespace fs = std::filesystem;
  using DataFrame = pluss::table::DataFrame;
  using GroupOp = DataFrame::GroupOp;

  const fs::path csv_path = fs::path(PLUSSMIC_SOURCE_DIR) / "data" / "sp500_data.csv";
  const fs::path out_dir = fs::path(PLUSSMIC_SOURCE_DIR) / "examples" / "outputs" / "sp500_eda_ml_prep_notebook";
  fs::create_directories(out_dir);
  fs::current_path(out_dir);

  //========== Cell One ===========
  // Head
  //==============End Cell One =============
  pluss::loader::ReadCsvOptions read_opts;
  read_opts.skipcases = {1, 2};
  read_opts.parse_dates = {"Price"};
  let df = DataFrame::load_csv(csv_path.string(), read_opts).rename("Price", "Date");
  std::cout << "\n[Cell 1] Raw shape: [" << df.ncases() << " cs x " << df.nfeats() << " feat]\n";
  df.head(8);

  //========== Cell Two ===========
  // Info
  //==============End Cell Two=============
  std::cout << "\n[Cell 2] Raw info + summary\n";
  df.info();
  df.describe();

  // Feature extraction
  df.set({"Year", "Month", "DayOfWeek", "DailyRange", "OpenCloseSpread", "LogVolume"}) = {
      df.features("Date").year(),
      df.features("Date").month(),
      df.features("Date").to_note_weekday(),
      df.features("High") - df.features("Low"),
      df.features("Close") - df.features("Open"),
      (df.features("Volume") + 1.0).log(),
  };

  std::cout << "\nCleaned shape: [" << df.ncases() << " cs x " << df.nfeats() << " feat]\n";
  df.head(5);

  //========== Cell Three ===========
  // Top three months with the most traded volumes
  //==============End Cell Three =============
  std::cout << "\n[Cell 3] Top (Year, Month) by traded volume\n";
  let monthly_volume = df.group_by({"Year", "Month"},
                                    {
                                        {"TotalVolume", "Volume", GroupOp::Sum},
                                        {"MeanClose", "Close", GroupOp::Mean},
                                    },
                                    true,
                                    false)
                            .sort_values({"TotalVolume"}, false);
  monthly_volume.head(3);

  //========== Cell Four ===========
  // plotting 2d histogram of data
  //==============End Cell Four =============
  std::cout << "\n[Cell 4] 2D plots -> PDF outputs\n";
  pluss::table::DataFramePlotOptions plot_opts;
  plot_opts.default_x_feature = "Date";
  plot_opts.use_tight_layout = true;
  plot_opts.tight_layout_padding = 4.0;
  plot_opts.width = 1280.0;
  plot_opts.height = 760.0;

  let fig_close_line = df.plot(plot_opts).line("Close");
  fig_close_line.headline("S&P 500 Close over time");
  fig_close_line.save_pdf("sp500_close_line.pdf");

  let fig_volume_hist = df.plot().hist("Volume", 80);
  fig_volume_hist.headline("Volume histogram");
  fig_volume_hist.save_pdf("sp500_volume_hist.pdf");

  let fig_scatter = df.plot().scatter("LogVolume", "DailyRange");
  fig_scatter.headline("Daily range vs log-volume");
  fig_scatter.save_pdf("sp500_range_vs_logvolume_scatter.pdf");

  let numeric = df.series_features();
  let corr_table = numeric.corr(DataFrame::CorrMethod::Spearman, false);
  corr_table.save_csv("sp500_corr_matrix_spearman.csv");
  corr_table.head(12);

  let fig_corr = numeric.plot().corr_heatmap();
  fig_corr.headline("Correlation heatmap (price/volume features)");
  fig_corr.save_pdf("sp500_corr_heatmap.pdf");

  let monthly_top24 = monthly_volume.head(24, false);
  let fig_monthly_bar = monthly_top24.plot().bar("TotalVolume");
  fig_monthly_bar.headline("Top 24 (Year, Month) by traded volume");
  fig_monthly_bar.save_pdf("sp500_top24_monthly_volume_bar.pdf");

  //========== Cell Five ===========
  // plotting 3d scatter
  //==============End Cell Five =============
  std::cout << "\n[Cell 5] 3D plot -> PDF/SVG outputs\n";
  let fig3_scatter = df.plot3d().scatter("Close", "LogVolume", "DailyRange");
  fig3_scatter.headline("S&P 500 3D scatter: close vs log-volume vs daily-range");
  fig3_scatter.save_pdf("sp500_3d_scatter.pdf");

  let fig3_line = df.plot3d().line("Close", "LogVolume", "DailyRange");
  fig3_line.headline("S&P 500 3D path: close vs log-volume vs daily-range");
  fig3_line.save_pdf("sp500_3d_path.pdf");

  let fig3_surface = df.plot3d().surface("Month", "Year", "Close", GroupOp::Mean);
  fig3_surface.headline("Monthly mean close surface (Month x Year)");
  fig3_surface.save_pdf("sp500_3d_surface_month_year_close.pdf");

  //========== Cell Six ===========
  // preparing data for ML regression task
  //==============End Cell Six =============
  std::cout << "\n[Cell 6] ML prep (features, target, split, export)\n";
  let ml_frame = df.features({"Date"}).combine(numeric);
  let close_prev_1 = numeric.shift({"Close"}, 1).features({"Close"}).rename("Close", "ClosePrev1");
  let close_prev_5 = numeric.shift({"Close"}, 5).features({"Close"}).rename("Close", "ClosePrev5");
  let volume_prev_1 = numeric.shift({"Volume"}, 1).features({"Volume"}).rename("Volume", "VolumePrev1");
  let close_next_1 = numeric.shift({"Close"}, -1).features({"Close"}).rename("Close", "CloseNext1");

  ml_frame = ml_frame.combine(close_prev_1).combine(close_prev_5).combine(volume_prev_1).combine(close_next_1).dropna();
  ml_frame.set({"Ret1D", "Ret5D", "VolumeChange1D", "TargetNextRet"}) = {
      (ml_frame.features("Close") / ml_frame.features("ClosePrev1")) - 1.0,
      (ml_frame.features("Close") / ml_frame.features("ClosePrev5")) - 1.0,
      (ml_frame.features("Volume") / ml_frame.features("VolumePrev1")) - 1.0,
      (ml_frame.features("CloseNext1") / ml_frame.features("Close")) - 1.0,
  };
  ml_frame = ml_frame.drop({"ClosePrev1", "ClosePrev5", "VolumePrev1", "CloseNext1"}).set_cases("Date", true);

  std::cout << "ML-ready rows: " << ml_frame.ncases() << "\n";
  ml_frame.head(6);

  const std::int64_t split = static_cast<std::int64_t>(static_cast<double>(ml_frame.ncases()) * 0.8);
  let train = ml_frame.slice(0, split);
  let valid = ml_frame.slice(split, ml_frame.ncases());

  std::cout << "Train rows: " << train.ncases() << ", Validation rows: " << valid.ncases() << "\n";
  train.describe();

  train.save_csv("sp500_ml_train.csv");
  valid.save_csv("sp500_ml_valid.csv");
  ml_frame.save_csv("sp500_ml_full.csv");

  let fig_target_hist = ml_frame.plot().hist("TargetNextRet", 120);
  fig_target_hist.headline("Target distribution: next-day return");
  fig_target_hist.save_pdf("sp500_target_next_return_hist.pdf");

  std::cout << "\nSaved outputs under: " << out_dir << "\n";
  std::cout << "Done. Data is ready for NodeJ/PlussAI training phase when available.\n";
  return 0;
}
