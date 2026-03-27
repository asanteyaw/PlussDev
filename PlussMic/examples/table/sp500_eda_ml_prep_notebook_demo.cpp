#include <pluss/pluss.h>
#include <pluss/tils.h>

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

  pluss::table::ScatterMatrixPlotOptions scat_opts;
  scat_opts.corner = true;
  scat_opts.pair.kind = pluss::pl::flash::PairPlotKind::Reg;
  scat_opts.pair.diag_kind = pluss::pl::flash::PairPlotDiagKind::Density;
  scat_opts.pair.dropna = true;
  scat_opts.pair.max_points = 1400;
  scat_opts.pair.show_grid = true;
  df.plot()
      .scatter_matrix({"Adj Close", "Close", "High", "Low", "Open", "Volume"}, scat_opts)
      .headline("scatter plot")
      .save_pdf("spx_scatter_matrix.pdf");

  // Feature extraction
  df.set({"Year", "Month", "DayOfWeek", "DailyRange", "OpenCloseSpread", "LogVolume"}) = {
      df.features("Date").year(),
      df.features("Date").month(),
      df.features("Date").as_timeline().strftime("%A"),
      df.features("High") - df.features("Low"),
      df.features("Close") - df.features("Open"),
      (df.features("Volume") + 1.0).log(),
  };

  std::cout << "\nCleaned shape: [" << df.ncases() << " cs x " << df.nfeats() << " feat]\n";
  df.features({"Year", "Month", "DayOfWeek", "DailyRange", "OpenCloseSpread", "LogVolume"}).head(5);

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
  plot_opts.max_labeled_ticks = 10;
  plot_opts.labeled_tick_rotation_deg = 55.0;

  df.plot(plot_opts).line("Close").headline("S&P 500 Close over time").save_pdf("sp500_close_line.pdf");

  df.plot().hist("Volume", 80).headline("Volume histogram").save_pdf("sp500_volume_hist.pdf");

  df.plot()
      .scatter("LogVolume", "DailyRange")
      .headline("Daily range vs log-volume")
      .save_pdf("sp500_range_vs_logvolume_scatter.pdf");


  let numeric = df.series_features();
  let corr_table = numeric.corr(DataFrame::CorrMethod::Spearman, false);
  corr_table.save_csv("sp500_corr_matrix_spearman.csv");
  corr_table.head(12);

  numeric.plot()
      .corr_heatmap()
      .headline("Correlation heatmap (price/volume features)")
      .save_pdf("sp500_corr_heatmap.pdf");

  std::cout << "\n[Cell 4B] Advanced plotters (parallel/andrews/radviz/lag/act/bootstrap/brief)\n";
  pluss::table::ParallelPlotOptions parallel_opts;
  parallel_opts.max_points = 700;
  df.plot()
      .parallel({"Adj Close", "Close", "High", "Low", "Open"}, "DayOfWeek", parallel_opts)
      .headline("Parallel coordinates by weekday")
      .save_pdf("sp500_parallel.pdf");

  pluss::table::AndrewsCurvesPlotOptions andrews_opts;
  andrews_opts.max_curves = 500;
  df.plot()
      .andrews_curves({"Adj Close", "Close", "High", "Low", "Open"}, "DayOfWeek", andrews_opts)
      .headline("Andrews curves by weekday")
      .save_pdf("sp500_andrews_curves.pdf");

  pluss::table::RadvizPlotOptions radviz_opts;
  radviz_opts.max_points = 3000;
  let fig_radviz = df.plot().radviz({"Adj Close", "Close", "High", "Low", "Open", "LogVolume"}, "DayOfWeek", radviz_opts);
  fig_radviz.headline("Radviz by weekday");
  fig_radviz.save_pdf("sp500_radviz.pdf");

  pluss::table::LagPlotOptions lag_opts;
  lag_opts.lag = 1;
  lag_opts.max_points = 2500;
  let fig_lag = df.plot().lag("Close", lag_opts);
  fig_lag.headline("Lag plot (Close, lag=1)");
  fig_lag.save_pdf("sp500_lag_close.pdf");

  pluss::table::ActPlotOptions acf_opts;
  acf_opts.max_lag = 60;
  df.plot().act("Close", acf_opts).headline("Autocorrelation plot (Close)").save_pdf("sp500_act_close.pdf");

  pluss::table::BootstrapPlotOptions boot_opts;
  boot_opts.samples = 320;
  boot_opts.sample_size = 512;
  boot_opts.bins = 28;
  df.plot()
      .bootstrap("Close", boot_opts)
      .headline("Bootstrap distribution (Close mean)")
      .save_pdf("sp500_bootstrap_close.pdf");

  pluss::table::BriefPlotOptions brief_opts;
  brief_opts.title = "Tables for plotted series (first cases)";
  brief_opts.precision = 2;
  brief_opts.max_cases = 8;
  brief_opts.features_per_table = 1;
  brief_opts.tables_per_row = 3;
  brief_opts.table_gap_spaces = 8;
  let fig_report = df.plot().line({"Close", "Open", "High", "Low", "Adj Close"}, "Date");
  fig_report.mosaic(2, 1, 12.0, 10.0).tight_layout(true, 4.0).size(1480.0, 920.0);
  df.plot().brief(fig_report, {"Close", "Open", "High", "Low", "Adj Close"}, brief_opts);
  fig_report.headline("Price trends + supporting tables");
  fig_report.save_pdf("sp500_close_with_brief_table.pdf");

  let monthly_top24 = monthly_volume.head(24, false);
  monthly_top24.plot()
      .bar("TotalVolume")
      .headline("Top 24 (Year, Month) by traded volume")
      .save_pdf("sp500_top24_monthly_volume_bar.pdf");
  pluss::table::BarPlotOptions hbar_opts;
  hbar_opts.horizontal = true;
  monthly_top24.plot()
      .bar("TotalVolume", std::nullopt, hbar_opts)
      .headline("Top 24 (Year, Month) by traded volume - horizontal")
      .save_pdf("sp500_top24_monthly_volume_barh.pdf");
  pluss::table::PiePlotOptions pie_opts;
  pie_opts.show_label = true;
  pie_opts.show_percent = true;
  pie_opts.show_value = true;
  pie_opts.precision = 1;
  monthly_top24.head(8, false)
      .plot()
      .pie("TotalVolume", "Month", pie_opts)
      .headline("Top 8 monthly traded volume share (by month label)")
      .save_pdf("sp500_top8_monthly_volume_pie.pdf");

  //========== Cell Five ===========
  // plotting 3d scatter
  //==============End Cell Five =============
  std::cout << "\n[Cell 5] 3D plot -> PDF/SVG outputs\n";
  pluss::table::Plot3DSceneOptions scene3_opts;
  scene3_opts.preset = pluss::pl::flash::ScenePreset3D::Presentation;
  scene3_opts.theme = pluss::pl::flash::ThemePreset3D::Slate;
  scene3_opts.camera_preset = pluss::pl::flash::CameraPreset3D::Isometric;
  scene3_opts.lighting = pluss::pl::flash::Lighting3DOptions{
      .clear_first = true,
      .headlight = true,
      .lamp_left = true,
      .sun_key = true,
  };
  pluss::table::Scatter3DPlotOptions scatter3_opts;
  scatter3_opts.scene = scene3_opts;
  scatter3_opts.trace.preset = pluss::pl::flash::TracePreset3D::Cloud;
  scatter3_opts.trace.material = pluss::pl::MaterialProfile::Shiny;
  scatter3_opts.trace.lighting = pluss::pl::LightingMode::Gouraud;
  scatter3_opts.trace.shade = pluss::pl::ShadeMode::Smooth;
  df.plot3d()
      .scatter("Close", "LogVolume", "DailyRange", std::nullopt, std::nullopt, scatter3_opts)
      .headline("S&P 500 3D scatter: close vs log-volume vs daily-range")
      .save_pdf("sp500_3d_scatter.pdf");

  pluss::table::Line3DPlotOptions line3_opts;
  line3_opts.scene = scene3_opts;
  line3_opts.trace.preset = pluss::pl::flash::TracePreset3D::Path;
  line3_opts.scene.camera = pluss::pl::flash::Camera3DOptions{
      .azimuth_deg = -48.0,
      .elevation_deg = 24.0,
      .view_angle_deg = 28.0,
      .projection = pluss::pl::ProjectionKind::Perspective,
  };
  line3_opts.trace.line_width = 1.8F;
  df.plot3d()
      .line("Close", "LogVolume", "DailyRange", line3_opts)
      .headline("S&P 500 3D path: close vs log-volume vs daily-range")
      .save_pdf("sp500_3d_path.pdf");

  pluss::table::Hist3DPlotOptions hist3d_opts;
  hist3d_opts.bins_x = 30;
  hist3d_opts.bins_y = 18;
  hist3d_opts.scene = scene3_opts;
  hist3d_opts.trace.preset = pluss::pl::flash::TracePreset3D::Spire;
  hist3d_opts.trace.material = pluss::pl::MaterialProfile::Metal;
  hist3d_opts.trace.lighting = pluss::pl::LightingMode::Gouraud;
  df.plot3d()
      .hist("Close", "Volume", hist3d_opts)
      .headline("S&P 500 3D histogram: Close x Volume")
      .save_pdf("sp500_3d_hist_close_volume.pdf");

  pluss::table::Surface3DPlotOptions surface3_opts;
  surface3_opts.scene = scene3_opts;
  surface3_opts.trace.preset = pluss::pl::flash::TracePreset3D::Surface;
  surface3_opts.trace.material = pluss::pl::MaterialProfile::Shiny;
  surface3_opts.trace.shade = pluss::pl::ShadeMode::Smooth;
  df.plot3d()
      .surface("Month", "Year", "Close", GroupOp::Mean, surface3_opts)
      .headline("Monthly mean close surface (Month x Year)")
      .save_pdf("sp500_3d_surface_month_year_close.pdf");

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

  ml_frame.plot()
      .hist("TargetNextRet", 120)
      .headline("Target distribution: next-day return")
      .save_pdf("sp500_target_next_return_hist.pdf");

  std::cout << "\nSaved outputs under: " << out_dir << "\n";
  std::cout << "Done. Data is ready for NodeJ/PlussAI training phase when available.\n";
  return 0;
}
