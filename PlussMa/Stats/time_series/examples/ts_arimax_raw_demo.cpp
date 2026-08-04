#include <pluss/ma.h>

#include <iostream>

int main() {
  using namespace pluss::subsp::stats;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const std::int64_t n = 512;
  const pluss::Series x = pluss::series::randn({n, 2}, cfg, 20260450ULL);

  const ts::ArimaFit truth{
      .mean = 0.0,
      .intercept = 0.08,
      .sigma = 0.60,
      .ar = {0.45},
      .ma = {},
      .sar = {},
      .sma = {},
      .beta = {0.70, -0.35},
      .diff_order = 0,
      .seasonal_diff_order = 0,
      .seasonal_period = 0,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };

  const pluss::Series y = truth.simulate(x, cfg, 20260451ULL);

  ts::ArimaOptions opt;
  opt.ar_order = 1;
  opt.beta0 = {0.0, 0.0};
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;
  const auto fit = ts::Arima::fit_mle(y, x, opt, cfg);

  const pluss::Series x_future = pluss::series::randn({12, 2}, cfg, 20260452ULL);

  std::cout << "ARIMAX\n";
  std::cout << "mean=" << fit.mean << " intercept=" << fit.intercept << " sigma=" << fit.sigma << "\n";
  std::cout << "ar=" << pluss::make_series(fit.ar, {static_cast<std::int64_t>(fit.ar.size())}, cfg) << "\n";
  std::cout << "beta=" << pluss::make_series(fit.beta, {static_cast<std::int64_t>(fit.beta.size())}, cfg) << "\n";
  std::cout << "loglik=" << fit.loglik(y, x) << "\n";
  std::cout << "loglik_per_obs=" << fit.loglik_per_obs(y, x, cfg) << "\n";
  std::cout << "infer=" << fit.infer(y, x, cfg) << "\n";
  std::cout << "score=" << fit.score(y, x, cfg) << "\n";
  std::cout << "observed_inf=" << fit.observed_inf(y, x, cfg) << "\n";
  std::cout << "forecast=" << fit.forecast(12, y, x, x_future, cfg) << "\n";
  std::cout << "simulate=" << fit.simulate(x, cfg, 20260453ULL) << "\n";

  return 0;
}
