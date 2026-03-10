#include <pluss/ma.h>

#include <iostream>

int main() {
  using namespace pluss::subsp::stats;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const ts::EgarchFit eg_truth{
      .mean = 0.0,
      .omega = -0.15,
      .arch = {0.12},
      .asym = {-0.06},
      .garch = {0.90},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const pluss::Series yeg = eg_truth.simulate(640, cfg, 20260423ULL);

  ts::EgarchOptions eopt;
  eopt.arch_order = 1;
  eopt.garch_order = 1;
  eopt.max_iterations = 1400;
  eopt.max_evaluations = 14000;
  const auto efit = ts::Egarch::fit_mle(yeg, eopt, cfg);

  std::cout << "EGARCH\n";
  std::cout << "mean=" << efit.mean << " omega=" << efit.omega << "\n";
  std::cout << "arch=" << pluss::make_series(efit.arch, {static_cast<std::int64_t>(efit.arch.size())}, cfg) << "\n";
  std::cout << "asym=" << pluss::make_series(efit.asym, {static_cast<std::int64_t>(efit.asym.size())}, cfg) << "\n";
  std::cout << "garch=" << pluss::make_series(efit.garch, {static_cast<std::int64_t>(efit.garch.size())}, cfg) << "\n";
  std::cout << "persistence=" << efit.persistence() << "\n";
  std::cout << "loglik=" << efit.loglik(yeg) << "\n";
  std::cout << "loglik_per_obs=" << efit.loglik_per_obs(yeg, cfg) << "\n";
  std::cout << "infer=" << efit.infer(yeg, cfg) << "\n";
  std::cout << "score=" << efit.score(yeg, cfg) << "\n";
  std::cout << "observed_inf=" << efit.observed_inf(yeg, cfg) << "\n";
  std::cout << "forecast=" << efit.forecast(12, yeg, cfg) << "\n";
  std::cout << "simulate=" << efit.simulate(24, cfg, 20260424ULL) << "\n";

  const ts::GjrFit gj_truth{
      .mean = 0.0,
      .omega = 0.02,
      .arch = {0.06},
      .asym = {0.08},
      .garch = {0.86},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const pluss::Series ygj = gj_truth.simulate(640, cfg, 20260425ULL);

  ts::GjrOptions gopt;
  gopt.arch_order = 1;
  gopt.garch_order = 1;
  gopt.max_iterations = 1400;
  gopt.max_evaluations = 14000;
  const auto gfit = ts::Gjr::fit_mle(ygj, gopt, cfg);

  std::cout << "\nGJR\n";
  std::cout << "mean=" << gfit.mean << " omega=" << gfit.omega << "\n";
  std::cout << "arch=" << pluss::make_series(gfit.arch, {static_cast<std::int64_t>(gfit.arch.size())}, cfg) << "\n";
  std::cout << "asym=" << pluss::make_series(gfit.asym, {static_cast<std::int64_t>(gfit.asym.size())}, cfg) << "\n";
  std::cout << "garch=" << pluss::make_series(gfit.garch, {static_cast<std::int64_t>(gfit.garch.size())}, cfg) << "\n";
  std::cout << "persistence=" << gfit.persistence() << "\n";
  std::cout << "unconditional_var=" << gfit.unconditional_var() << "\n";
  std::cout << "loglik=" << gfit.loglik(ygj) << "\n";
  std::cout << "loglik_per_obs=" << gfit.loglik_per_obs(ygj, cfg) << "\n";
  std::cout << "infer=" << gfit.infer(ygj, cfg) << "\n";
  std::cout << "score=" << gfit.score(ygj, cfg) << "\n";
  std::cout << "observed_inf=" << gfit.observed_inf(ygj, cfg) << "\n";
  std::cout << "forecast=" << gfit.forecast(12, ygj, cfg) << "\n";
  std::cout << "simulate=" << gfit.simulate(24, cfg, 20260426ULL) << "\n";

  return 0;
}
