#include <pluss/ma.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 1800;
  const auto x = pluss::randn({n, 2}, cfg, 20260410ULL);
  const auto xv = x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().to_vec<double>();
  const auto eps = pluss::randn({n}, cfg, 20260411ULL).to_vec<double>();

  std::vector<double> yv(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    const double x0_lag = xv[static_cast<std::size_t>((t - 1) * 2)];
    const double x1_lag = xv[static_cast<std::size_t>((t - 1) * 2 + 1)];
    yv[static_cast<std::size_t>(t)] =
        0.62 * yv[static_cast<std::size_t>(t - 1)] + 0.22 * x0_lag - 0.11 * x1_lag + 0.03 * eps[static_cast<std::size_t>(t)];
  }
  const auto y = pluss::make_series(yv, {n}, cfg);

  pluss::subsp::stats::ts::PartitionOptions popts;
  popts.direction = pluss::subsp::stats::ts::Direction::Forward;
  popts.train_size = 700;
  popts.test_size = 420;
  popts.step_size = 210;
  popts.max_lag = 8;
  const auto part = pluss::subsp::stats::ts::TsPartition::sliding_window(n, 4, popts);

  pluss::subsp::stats::ts::DirectForecasterOptions opts;
  opts.lag.horizon = {1, 2, 4};
  opts.lag.response_lags = {1, 2, 3};
  opts.lag.predictor_lags = {{1, 2}};
  opts.fit_intercept = true;
  opts.learner = pluss::subsp::stats::ts::DirectLearnerKind::Ridge;
  opts.ridge_lambda = 1e-4;

  const auto cv = pluss::subsp::stats::ts::DirectCVForecaster::fit(x, y, part, opts, cfg);
  const auto pred = cv.cvpredict(x, y, cfg);
  const auto loss_avg = cv.cvloss(
      x, y, pluss::subsp::stats::ts::CVLossMode::Average, pluss::subsp::stats::ts::RegressionLoss::MSE, cfg);
  const auto loss_ind = cv.cvloss(
      x, y, pluss::subsp::stats::ts::CVLossMode::Individual, pluss::subsp::stats::ts::RegressionLoss::MSE, cfg);
  const auto loss_avg_mae = cv.cvloss(
      x, y, pluss::subsp::stats::ts::CVLossMode::Average, pluss::subsp::stats::ts::RegressionLoss::MAE, cfg);

  std::cout << "DirectCVForecaster\n";
  std::cout << "num_folds=" << cv.num_folds() << " num_steps=" << cv.num_steps() << "\n";
  for (std::int64_t i = 0; i < cv.num_folds(); ++i) {
    const auto& w = cv.partition().window(i);
    std::cout << "fold=" << i
              << " train=[" << w.train_start << "," << w.train_end << "]"
              << " test=[" << w.test_start << "," << w.test_end << "]\n";
  }

  std::cout << "\ncvpredict\n" << pred << "\n\n";
  std::cout << "cvloss average (mse)\n" << loss_avg << "\n\n";
  std::cout << "cvloss individual (mse)\n" << loss_ind << "\n\n";
  std::cout << "cvloss average (mae)\n" << loss_avg_mae << "\n";
  return 0;
}
