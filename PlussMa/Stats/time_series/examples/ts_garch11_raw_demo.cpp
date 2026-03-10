#include <pluss/pluss.h>

#include <iostream>

int main() {
  using namespace pluss::subsp::stat;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const ts::Garch11Fit truth{
      .mean = 0.0,
      .omega = 0.02,
      .alpha = 0.08,
      .beta = 0.90,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const pluss::Series y = truth.simulate(128, cfg, 20260327ULL);

  ts::Garch11Options opt;
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;
  const auto fit = ts::Garch11::fit_mle(y, opt, cfg);

  std::cout << "Garch11\n";
  std::cout << "mean=" << fit.mean << " omega=" << fit.omega
            << " alpha=" << fit.alpha << " beta=" << fit.beta << "\n";
  std::cout << "unconditional_var=" << fit.unconditional_var() << "\n";
  std::cout << "loglik=" << fit.loglik(y) << "\n";
  std::cout << "loglik_per_obs=" << fit.loglik_per_obs(y, cfg) << "\n";
  std::cout << "infer=" << fit.infer(y, cfg) << "\n";
  std::cout << "score=" << fit.score(y, cfg) << "\n";
  std::cout << "observed_inf=" << fit.observed_inf(y, cfg) << "\n";
  std::cout << "forecast=" << fit.forecast(12, y, cfg) << "\n";
  std::cout << "simulate=" << fit.simulate(24, cfg, 20260328ULL) << "\n";
  return 0;
}
