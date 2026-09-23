#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {0.0, 10.0, 20.0, 30.0, 40.0, 500.0, 510.0}, {7}, f64).reshape({7, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {7}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  batch.sample_weights = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 1.0, 2.0}, {7}, f64);

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Counts;
  target.counts = pluss::make_series({7.0, 3.0}, {2}, f64);
  target.growth = cls::BalanceGrowthKind::BlendNeighbors;
  target.neighbors = 2;
  cls::RandomState random{};
  random.seed = 17;

  const cls::BalancePlan plan = cls::plan_balance(batch, target, random).value();
  const cls::LabeledBatch balanced = cls::balance_rows(batch, target, random).value();

  const std::int64_t base_idx = plan.synthetic_base_indices.slice(0, 0, 1).item<std::int64_t>();
  const std::int64_t neighbor_idx = plan.synthetic_neighbor_indices.slice(0, 0, 1).item<std::int64_t>();
  const double w_base = batch.sample_weights.slice(0, base_idx, base_idx + 1).item<double>();
  const double w_neighbor = batch.sample_weights.slice(0, neighbor_idx, neighbor_idx + 1).item<double>();
  const std::int64_t synthesized_row = plan.indices.numel();
  const double actual_w = balanced.sample_weights.slice(0, synthesized_row, synthesized_row + 1).item<double>();

  std::cout << "expected ((w_base + w_neighbor) / 2) = " << (w_base + w_neighbor) / 2.0 << "\n";
  std::cout << "actual                                = " << actual_w << "\n";

  return 0;
}
