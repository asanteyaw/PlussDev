#include <pluss/ma.h>

#include <iostream>

int main() {
  using namespace pluss::subsp::stats;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const ts::GarchFit truth{
      .mean = 0.0,
      .omega = 0.015,
      .arch = {0.06, 0.03},
      .garch = {0.84},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const pluss::Series y = truth.simulate(256, cfg, 20260331ULL);

  ts::GarchOptions opt;
  opt.arch_order = 2;
  opt.garch_order = 1;
  opt.arch0 = {0.05, 0.05};
  opt.garch0 = {0.85};
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;
  const auto fit = ts::Garch::fit_mle(y, opt, cfg);

  std::cout << "Garch(p,q)\n";
  std::cout << "mean=" << fit.mean << " omega=" << fit.omega << "\n";
  std::cout << "arch=" << pluss::make_series(fit.arch, {static_cast<std::int64_t>(fit.arch.size())}, cfg) << "\n";
  std::cout << "garch=" << pluss::make_series(fit.garch, {static_cast<std::int64_t>(fit.garch.size())}, cfg) << "\n";
  std::cout << "persistence=" << fit.persistence() << "\n";
  std::cout << "unconditional_var=" << fit.unconditional_var() << "\n";
  std::cout << "loglik=" << fit.loglik(y) << "\n";
  std::cout << "loglik_per_obs=" << fit.loglik_per_obs(y, cfg) << "\n";
  std::cout << "infer=" << fit.infer(y, cfg) << "\n";
  std::cout << "score=" << fit.score(y, cfg) << "\n";
  std::cout << "observed_inf=" << fit.observed_inf(y, cfg) << "\n";
  std::cout << "forecast=" << fit.forecast(12, y, cfg) << "\n";
  std::cout << "simulate=" << fit.simulate(24, cfg, 20260401ULL) << "\n";
  return 0;
}
