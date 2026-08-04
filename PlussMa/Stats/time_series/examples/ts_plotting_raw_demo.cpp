#include <pluss/ma.h>

#include <pluss/pl/studio.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  using namespace pluss::subsp::stats;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 1800;

  const auto eps = pluss::series::randn({n}, cfg, 20260411ULL).to_vec<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    y[static_cast<std::size_t>(t)] = 0.70 * y[static_cast<std::size_t>(t - 1)] + eps[static_cast<std::size_t>(t)];
  }

  const pluss::Series ys = pluss::make_series(y, {n}, cfg);

  ts::plot::CorrPlotOptions corr_opt;
  corr_opt.max_lag = 24;
  corr_opt.norm = ts::AcfNorm::Unbiased;
  corr_opt.include_zero_lag = false;
  corr_opt.confidence_level = 0.95;

  const auto acf_payload = ts::plot::acf_payload(ys, corr_opt, cfg);
  const auto pacf_payload = ts::plot::pacf_payload(ys, corr_opt, cfg);
  const auto resid_payload = ts::plot::residuals_payload(ys, cfg);

  std::cout << "ACF payload lags\n" << acf_payload.lags << "\n\n";
  std::cout << "ACF payload values\n" << acf_payload.values << "\n";
  std::cout << "ACF confidence band: [" << acf_payload.conf_low << ", " << acf_payload.conf_high << "]\n\n";

  std::cout << "PACF payload lags\n" << pacf_payload.lags << "\n\n";
  std::cout << "PACF payload values\n" << pacf_payload.values << "\n";
  std::cout << "PACF confidence band: [" << pacf_payload.conf_low << ", " << pacf_payload.conf_high << "]\n\n";

  std::cout << "Residual payload index\n" << resid_payload.index << "\n\n";
  std::cout << "Residual payload values\n" << resid_payload.residuals << "\n\n";

  pluss::pl::Studio acf_fig("ts-acf-raw-demo", pluss::pl::Theme::paper());
  ts::plot::acf(acf_fig, ys, corr_opt, cfg);

  pluss::pl::Studio pacf_fig("ts-pacf-raw-demo", pluss::pl::Theme::paper());
  ts::plot::pacf(pacf_fig, ys, corr_opt, cfg);

  ts::plot::ResidualPlotOptions resid_opt;
  pluss::pl::Studio resid_fig("ts-residuals-raw-demo", pluss::pl::Theme::paper());
  ts::plot::residuals(resid_fig, ys, resid_opt, cfg);

  // acf_fig.save_svg("PlussMa/Stats/time_series/examples/outputs/ts_plot_acf_raw_demo.svg");
  // pacf_fig.save_svg("PlussMa/Stats/time_series/examples/outputs/ts_plot_pacf_raw_demo.svg");
  // resid_fig.save_svg("PlussMa/Stats/time_series/examples/outputs/ts_plot_residuals_raw_demo.svg");

  return 0;
}
