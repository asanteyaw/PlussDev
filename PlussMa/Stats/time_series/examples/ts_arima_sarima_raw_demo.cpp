#include <pluss/pluss.h>

#include <iostream>

int main() {
  using namespace pluss::subsp::stat;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const ts::ArimaFit ar_truth{
      .mean = 0.0,
      .intercept = 0.10,
      .sigma = 0.85,
      .ar = {0.55},
      .ma = {-0.20},
      .sar = {},
      .sma = {},
      .diff_order = 0,
      .seasonal_diff_order = 0,
      .seasonal_period = 0,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };

  const pluss::Series yar = ar_truth.simulate(512, cfg, 20260419ULL);
  ts::ArimaOptions aopt;
  aopt.ar_order = 1;
  aopt.ma_order = 1;
  aopt.max_iterations = 1200;
  aopt.max_evaluations = 12000;
  const auto afit = ts::Arima::fit_mle(yar, aopt, cfg);

  std::cout << "ARIMA\n";
  std::cout << "mean=" << afit.mean << " intercept=" << afit.intercept << " sigma=" << afit.sigma << "\n";
  std::cout << "ar=" << pluss::make_series(afit.ar, {static_cast<std::int64_t>(afit.ar.size())}, cfg) << "\n";
  std::cout << "ma=" << pluss::make_series(afit.ma, {static_cast<std::int64_t>(afit.ma.size())}, cfg) << "\n";
  std::cout << "loglik=" << afit.loglik(yar) << "\n";
  std::cout << "loglik_per_obs=" << afit.loglik_per_obs(yar, cfg) << "\n";
  std::cout << "infer=" << afit.infer(yar, cfg) << "\n";
  std::cout << "score=" << afit.score(yar, cfg) << "\n";
  std::cout << "observed_inf=" << afit.observed_inf(yar, cfg) << "\n";
  std::cout << "forecast=" << afit.forecast(12, yar, cfg) << "\n";
  std::cout << "simulate=" << afit.simulate(24, cfg, 20260420ULL) << "\n";

  const ts::ArimaFit sar_truth{
      .mean = 0.0,
      .intercept = 0.05,
      .sigma = 0.70,
      .ar = {0.20},
      .ma = {},
      .sar = {0.35},
      .sma = {0.10},
      .diff_order = 0,
      .seasonal_diff_order = 0,
      .seasonal_period = 12,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };

  const pluss::Series ysar = sar_truth.simulate(768, cfg, 20260421ULL);
  ts::SarimaOptions sopt;
  sopt.p = 1;
  sopt.P = 1;
  sopt.Q = 1;
  sopt.s = 12;
  sopt.max_iterations = 1500;
  sopt.max_evaluations = 15000;
  const auto sfit = ts::Sarima::fit_mle(ysar, sopt, cfg);

  std::cout << "\nSARIMA\n";
  std::cout << "mean=" << sfit.core.mean << " intercept=" << sfit.core.intercept << " sigma=" << sfit.core.sigma << "\n";
  std::cout << "ar=" << pluss::make_series(sfit.core.ar, {static_cast<std::int64_t>(sfit.core.ar.size())}, cfg) << "\n";
  std::cout << "sar=" << pluss::make_series(sfit.core.sar, {static_cast<std::int64_t>(sfit.core.sar.size())}, cfg) << "\n";
  std::cout << "sma=" << pluss::make_series(sfit.core.sma, {static_cast<std::int64_t>(sfit.core.sma.size())}, cfg) << "\n";
  std::cout << "loglik=" << sfit.loglik(ysar) << "\n";
  std::cout << "loglik_per_obs=" << sfit.loglik_per_obs(ysar, cfg) << "\n";
  std::cout << "infer=" << sfit.infer(ysar, cfg) << "\n";
  std::cout << "score=" << sfit.score(ysar, cfg) << "\n";
  std::cout << "observed_inf=" << sfit.observed_inf(ysar, cfg) << "\n";
  std::cout << "forecast=" << sfit.forecast(18, ysar, cfg) << "\n";
  std::cout << "simulate=" << sfit.simulate(24, cfg, 20260422ULL) << "\n";

  return 0;
}
