#include <pluss/pluss.h>

#include <filesystem>

int main() {
  namespace fs = std::filesystem;
  using DataFrame = pluss::table::DataFrame;
  using GroupOp = DataFrame::GroupOp;
  constexpr double kSpot = 100.0;

  const fs::path out_dir =
      fs::path(PLUSSMIC_SOURCE_DIR) / "examples" / "outputs" / "option_smile_skew_surface_demo";
  fs::create_directories(out_dir);
  fs::current_path(out_dir);

  const std::vector<double> strike{80.0, 90.0, 100.0, 110.0, 120.0};
  const std::vector<double> moneyness{0.8, 0.9, 1.0, 1.1, 1.2};
  const std::vector<double> iv_1m{0.34, 0.29, 0.25, 0.28, 0.33};
  const std::vector<double> iv_3m{0.31, 0.27, 0.24, 0.26, 0.3};
  const std::vector<double> iv_6m{0.29, 0.26, 0.23, 0.25, 0.28};

  auto smile_df = DataFrame({
      {"Strike", pluss::make_series(strike)},
      {"Moneyness", pluss::make_series(moneyness)},
      {"IV_1M", pluss::make_series(iv_1m)},
      {"IV_3M", pluss::make_series(iv_3m)},
      {"IV_6M", pluss::make_series(iv_6m)},
  });

  smile_df.plot()
      .line({"IV_1M", "IV_3M", "IV_6M"}, "Strike")
      .headline("Vol smile by maturity (strike)")
      .save_pdf("smile_by_strike.pdf");

  smile_df.plot()
      .line({"IV_1M", "IV_3M", "IV_6M"}, "Moneyness")
      .headline("Vol smile by maturity (moneyness = strike / spot)")
      .save_pdf("smile_by_moneyness.pdf");

  auto skew_df = DataFrame({
      {"Delta", pluss::make_series({0.1, 0.25, 0.5, 0.75, 0.9})},
      {"ImpliedVol", pluss::make_series({0.33, 0.29, 0.25, 0.23, 0.22})},
  });

  skew_df.plot()
      .line("ImpliedVol", "Delta")
      .headline("Vol skew (3M, by delta)")
      .save_pdf("skew_line.pdf");

  auto surface_df = DataFrame({
      {"MaturityMonths", pluss::make_series({1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 6, 6, 6, 6, 6})},
      {"Strike", pluss::make_series({
                     80.0, 90.0, 100.0, 110.0, 120.0,
                     80.0, 90.0, 100.0, 110.0, 120.0,
                     80.0, 90.0, 100.0, 110.0, 120.0,
                 })},
      {"ImpliedVol", pluss::make_series({
                        0.34, 0.29, 0.25, 0.28, 0.33,
                        0.31, 0.27, 0.24, 0.26, 0.3,
                        0.29, 0.26, 0.23, 0.25, 0.28,
                    })},
  });
  surface_df.set({"Moneyness"}) = {surface_df.features("Strike") / kSpot};

  surface_df.plot3d()
      .surface("MaturityMonths", "Strike", "ImpliedVol", GroupOp::Mean)
      .headline("Implied vol surface")
      .save_pdf("surface_3d.pdf");

  surface_df.plot()
      .heatmap("MaturityMonths", "Strike", "ImpliedVol", GroupOp::Mean)
      .headline("Implied vol surface heatmap")
      .save_pdf("surface_heatmap.pdf");

  return 0;
}
