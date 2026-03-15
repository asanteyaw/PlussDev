#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::subsp::stats::ts::JohansenOptions;
using pluss::subsp::stats::ts::JohansenStat;
using pluss::subsp::stats::ts::Vecm;
using pluss::subsp::stats::ts::VecmFit;
using pluss::subsp::stats::ts::VecmOptions;
using pluss::subsp::stats::ts::johansen_test;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

VecmFit truth_model() {
  const auto cfg = f64_cpu();
  VecmFit fit;
  fit.lag_order = 2;
  fit.rank = 1;
  fit.include_intercept = true;
  fit.include_trend = false;
  fit.alpha = pluss::make_series(std::vector<double>{-0.55, 0.42}, {2, 1}, cfg);
  fit.beta = pluss::make_series(std::vector<double>{1.0, -1.0}, {2, 1}, cfg);
  fit.impact = fit.alpha.matmul(fit.beta.transpose(0, 1));
  fit.gamma = pluss::make_series(
      std::vector<double>{
          0.08, 0.01,
         -0.03, 0.05,
      },
      {1, 2, 2},
      cfg);
  fit.constant = pluss::make_series(std::vector<double>{0.012, -0.007}, {2}, cfg);
  fit.trend = pluss::zeros({2}, cfg);
  fit.covariance = pluss::make_series(std::vector<double>{0.22, 0.03, 0.03, 0.18}, {2, 2}, cfg);
  return fit;
}

void test_vecm_rank_fit() {
  const auto cfg = f64_cpu();
  const auto truth = truth_model();
  const auto y = truth.simulate(3200, cfg, 20260326ULL);

  JohansenOptions jopt;
  jopt.lag_order = 2;
  jopt.include_intercept = true;
  jopt.include_trend = false;
  jopt.statistic = JohansenStat::Trace;
  jopt.alpha = 0.05;

  const auto j = johansen_test(y, jopt, cfg);
  expect(j.selected_rank >= 1, "johansen should detect positive rank");

  VecmOptions opt;
  opt.lag_order = 2;
  opt.rank = -1;
  opt.include_intercept = true;
  opt.include_trend = false;
  opt.alpha = 0.05;
  opt.ridge = 1e-8;
  opt.pinv_rcond = 1e-12;

  const auto fit = Vecm::fit_mle(y, opt, cfg);
  expect(fit.rank == j.selected_rank, "vecm rank should follow auto-selected johansen rank");
  expect(fit.infer(y, cfg).isfinite().all().item<double>() == 1.0, "vecm infer should be finite");
  expect(fit.loglik_per_obs(y, cfg).isfinite().all().item<double>() == 1.0, "vecm loglik_per_obs should be finite");
  expect(std::isfinite(fit.loglik(y)), "vecm loglik should be finite");
}

void test_vecm_forecast_sim() {
  const auto cfg = f64_cpu();
  const auto truth = truth_model();
  const auto y = truth.simulate(1400, cfg, 20260327ULL);

  VecmOptions opt;
  opt.lag_order = 2;
  opt.rank = 1;
  opt.include_intercept = true;
  opt.include_trend = false;

  const auto fit = Vecm::fit_mle(y, opt, cfg);
  const auto f = fit.forecast(10, y, cfg);
  const auto s = fit.simulate(260, cfg, 20260328ULL);

  expect(f.sizes().size() == 2 && f.sizes()[0] == 10 && f.sizes()[1] == 2, "vecm forecast shape mismatch");
  expect(s.sizes().size() == 2 && s.sizes()[0] == 260 && s.sizes()[1] == 2, "vecm simulate shape mismatch");
  expect(f.isfinite().all().item<double>() == 1.0, "vecm forecast should be finite");
  expect(s.isfinite().all().item<double>() == 1.0, "vecm simulate should be finite");
}

} // namespace

int main() {
  try {
    test_vecm_rank_fit();
    test_vecm_forecast_sim();
    std::cout << "ts_vecm_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "ts_vecm_test failed: " << e.what() << '\n';
    return 1;
  }
}
