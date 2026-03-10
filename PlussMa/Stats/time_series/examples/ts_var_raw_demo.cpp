#include <pluss/ma.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  using pluss::subsp::stats::ts::Var;
  using pluss::subsp::stats::ts::VarFit;
  using pluss::subsp::stats::ts::VarOptions;
  using pluss::subsp::stats::ts::var_to_vecm;
  using pluss::subsp::stats::ts::vecm_to_var;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  VarFit truth;
  truth.lag_order = 2;
  truth.include_intercept = true;
  truth.include_trend = false;
  truth.constant = pluss::make_series(std::vector<double>{0.10, -0.05}, {2}, cfg);
  truth.trend = pluss::zeros({2}, cfg);
  truth.ar = pluss::make_series(
      std::vector<double>{
          0.45, 0.08,
         -0.04, 0.35,
         -0.10, 0.03,
          0.02,-0.15,
      },
      {2, 2, 2},
      cfg);
  truth.beta = pluss::empty({0, 2}, cfg);
  truth.covariance = pluss::make_series(std::vector<double>{0.35, 0.08, 0.08, 0.25}, {2, 2}, cfg);

  const pluss::Series y = truth.simulate(2000, cfg, 20260611ULL);

  VarOptions opt;
  opt.lag_order = 2;
  opt.include_intercept = true;
  opt.include_trend = false;
  opt.ridge = 1e-8;

  const auto fit = Var::fit_mle(y, opt, cfg);

  std::cout << "VAR\n";
  std::cout << "constant=" << fit.constant << "\n";
  std::cout << "trend=" << fit.trend << "\n";
  std::cout << "ar=" << fit.ar << "\n";
  std::cout << "covariance=" << fit.covariance << "\n";
  std::cout << "sample_size=" << fit.sample_size << "\n";

  std::cout << "loglik=" << fit.loglik(y) << "\n";
  std::cout << "loglik_per_obs=" << fit.loglik_per_obs(y, cfg) << "\n";
  std::cout << "infer=" << fit.infer(y, cfg) << "\n";
  std::cout << "forecast=" << fit.forecast(12, y, cfg) << "\n";

  const auto vecm = var_to_vecm(fit.ar, cfg);
  std::cout << "vec_short_run=" << vecm.first << "\n";
  std::cout << "vec_impact=" << vecm.second << "\n";
  std::cout << "var_back=" << vecm_to_var(vecm.first, vecm.second, cfg) << "\n";

  return 0;
}
