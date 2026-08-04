#include <pluss/ma.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  using pluss::subsp::stats::ts::JohansenOptions;
  using pluss::subsp::stats::ts::JohansenStat;
  using pluss::subsp::stats::ts::Vecm;
  using pluss::subsp::stats::ts::VecmFit;
  using pluss::subsp::stats::ts::VecmOptions;
  using pluss::subsp::stats::ts::johansen_test;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  VecmFit truth;
  truth.lag_order = 2;
  truth.rank = 1;
  truth.include_intercept = true;
  truth.include_trend = false;
  truth.alpha = pluss::make_series(std::vector<double>{-0.55, 0.42}, {2, 1}, cfg);
  truth.beta = pluss::make_series(std::vector<double>{1.0, -1.0}, {2, 1}, cfg);
  truth.impact = truth.alpha.matmul(truth.beta.transpose(0, 1));
  truth.gamma = pluss::make_series(
      std::vector<double>{
          0.08, 0.01,
         -0.03, 0.05,
      },
      {1, 2, 2},
      cfg);
  truth.constant = pluss::make_series(std::vector<double>{0.012, -0.007}, {2}, cfg);
  truth.trend = pluss::series::zeros({2}, cfg);
  truth.covariance = pluss::make_series(std::vector<double>{0.22, 0.03, 0.03, 0.18}, {2, 2}, cfg);

  const auto y = truth.simulate(2800, cfg, 20260324ULL);

  JohansenOptions jopt;
  jopt.lag_order = 2;
  jopt.include_intercept = true;
  jopt.include_trend = false;
  jopt.statistic = JohansenStat::Trace;
  jopt.alpha = 0.05;
  const auto j = johansen_test(y, jopt, cfg);

  std::cout << "Johansen\n";
  std::cout << "eigenvalues=" << j.eigenvalues << "\n";
  std::cout << "stat=" << j.stat << "\n";
  std::cout << "pvalue=" << j.pvalue << "\n";
  std::cout << "critical=" << j.critical << "\n";
  std::cout << "reject=" << j.reject << "\n";
  std::cout << "selected_rank=" << j.selected_rank << "\n";
  std::cout << "alpha=" << j.alpha << "\n";
  std::cout << "beta=" << j.beta << "\n";

  VecmOptions opt;
  opt.lag_order = 2;
  opt.rank = -1;
  opt.include_intercept = true;
  opt.include_trend = false;
  opt.alpha = 0.05;
  opt.ridge = 1e-8;
  opt.pinv_rcond = 1e-12;
  const auto fit = Vecm::fit_mle(y, opt, cfg);

  std::cout << "\nVECM\n";
  std::cout << "lag_order=" << fit.lag_order << " rank=" << fit.rank << "\n";
  std::cout << "constant=" << fit.constant << "\n";
  std::cout << "trend=" << fit.trend << "\n";
  std::cout << "alpha=" << fit.alpha << "\n";
  std::cout << "beta=" << fit.beta << "\n";
  std::cout << "impact=" << fit.impact << "\n";
  std::cout << "gamma=" << fit.gamma << "\n";
  std::cout << "covariance=" << fit.covariance << "\n";
  std::cout << "sample_size=" << fit.sample_size << "\n";

  std::cout << "loglik=" << fit.loglik(y) << "\n";
  std::cout << "loglik_per_obs=" << fit.loglik_per_obs(y, cfg) << "\n";
  std::cout << "infer=" << fit.infer(y, cfg) << "\n";
  std::cout << "forecast=" << fit.forecast(12, y, cfg) << "\n";
  std::cout << "simulate=" << fit.simulate(220, cfg, 20260325ULL) << "\n";

  return 0;
}
