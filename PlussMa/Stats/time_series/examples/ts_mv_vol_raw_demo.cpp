#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace ts = pluss::subsp::stats::ts;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const ts::Garch11Fit g1{
      .mean = 0.0,
      .omega = 0.015,
      .alpha = 0.07,
      .beta = 0.90,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const ts::Garch11Fit g2{
      .mean = 0.0,
      .omega = 0.020,
      .alpha = 0.09,
      .beta = 0.86,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };

  const pluss::Series y1 = g1.simulate(700, cfg, 20270201ULL);
  const pluss::Series y2 = g2.simulate(700, cfg, 20270202ULL).mul(0.75).add(y1.mul(0.25));
  const pluss::Series y = y1.reshape({700, 1}).unite({y2.reshape({700, 1})}, 1);

  ts::MvGarchOptions gopt;
  gopt.marginal.max_iterations = 280;
  gopt.marginal.max_evaluations = 2600;
  gopt.marginal.nm_restarts = 1;
  const auto gfit = ts::MvGarch::fit_mle(y, gopt, cfg);

  std::cout << "\nMvGarch\n";
  std::cout << "mean=" << gfit.mean << "\n";
  std::cout << "corr=" << gfit.corr << "\n";
  std::cout << "loglik=" << gfit.loglik(y) << "\n";
  std::cout << "loglik_per_obs=" << gfit.loglik_per_obs(y, cfg) << "\n";
  std::cout << "infer=" << gfit.infer(y, cfg) << "\n";
  std::cout << "forecast=" << gfit.forecast(12, y, cfg) << "\n";

  const ts::EgarchFit e1{
      .mean = 0.0,
      .omega = -0.12,
      .arch = {0.11},
      .asym = {-0.04},
      .garch = {0.89},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const ts::EgarchFit e2{
      .mean = 0.0,
      .omega = -0.08,
      .arch = {0.09},
      .asym = {-0.02},
      .garch = {0.91},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };

  const pluss::Series ye1 = e1.simulate(700, cfg, 20270203ULL);
  const pluss::Series ye2 = e2.simulate(700, cfg, 20270204ULL).mul(0.8).add(ye1.mul(0.2));
  const pluss::Series ye = ye1.reshape({700, 1}).unite({ye2.reshape({700, 1})}, 1);

  ts::MvEgarchOptions eopt;
  eopt.marginal.max_iterations = 240;
  eopt.marginal.max_evaluations = 2400;
  eopt.marginal.nm_restarts = 1;
  const auto efit = ts::MvEgarch::fit_mle(ye, eopt, cfg);

  std::cout << "\nMvEgarch\n";
  std::cout << "mean=" << efit.mean << "\n";
  std::cout << "corr=" << efit.corr << "\n";
  std::cout << "loglik=" << efit.loglik(ye) << "\n";
  std::cout << "loglik_per_obs=" << efit.loglik_per_obs(ye, cfg) << "\n";
  std::cout << "infer=" << efit.infer(ye, cfg) << "\n";
  std::cout << "forecast=" << efit.forecast(12, ye, cfg) << "\n";

  const ts::GjrFit j1{
      .mean = 0.0,
      .omega = 0.015,
      .arch = {0.06},
      .asym = {0.06},
      .garch = {0.87},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const ts::GjrFit j2{
      .mean = 0.0,
      .omega = 0.018,
      .arch = {0.07},
      .asym = {0.05},
      .garch = {0.86},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };

  const pluss::Series yj1 = j1.simulate(700, cfg, 20270205ULL);
  const pluss::Series yj2 = j2.simulate(700, cfg, 20270206ULL).mul(0.8).add(yj1.mul(0.2));
  const pluss::Series yj = yj1.reshape({700, 1}).unite({yj2.reshape({700, 1})}, 1);

  ts::MvGjrOptions jopt;
  jopt.marginal.max_iterations = 240;
  jopt.marginal.max_evaluations = 2400;
  jopt.marginal.nm_restarts = 1;
  const auto jfit = ts::MvGjr::fit_mle(yj, jopt, cfg);

  std::cout << "\nMvGjr\n";
  std::cout << "mean=" << jfit.mean << "\n";
  std::cout << "corr=" << jfit.corr << "\n";
  std::cout << "loglik=" << jfit.loglik(yj) << "\n";
  std::cout << "loglik_per_obs=" << jfit.loglik_per_obs(yj, cfg) << "\n";
  std::cout << "infer=" << jfit.infer(yj, cfg) << "\n";
  std::cout << "forecast=" << jfit.forecast(12, yj, cfg) << "\n";

  return 0;
}
