#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const std::vector<std::int64_t> rows_v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      10, 1, rows_v, std::vector<std::int64_t>(10, 0),
      {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.2, 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0},
      {10}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 5;
  split.random.seed = 0;

  const cls::SparseCrossReport report = cls::cross_assess_sparse_regression(x, y, split).value();
  std::cout << "metrics.size() = " << report.metrics.size() << "\n";
  std::cout << "mean_metrics [Mse, Rmse, R2] = " << report.mean_metrics << "\n";

  return 0;
}
