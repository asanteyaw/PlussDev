#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // both classes have >= 2 rows, so every extra row here is genuinely
  // synthesized -- no class falls back to plain duplication, which keeps
  // plan.indices.numel() equal to the original row count (7), making the
  // very first synthetic row land at output index 7.
  const pluss::Series x = pluss::make_series(
      {0.0, 10.0, 20.0, 30.0, 40.0, 500.0, 510.0}, {7}, f64).reshape({7, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {7}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

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
  const double step = plan.synthetic_steps.slice(0, 0, 1).item<double>();
  const double base_val = x.slice(0, base_idx, base_idx + 1).item<double>();
  const double neighbor_val = x.slice(0, neighbor_idx, neighbor_idx + 1).item<double>();
  const double expected = base_val + step * (neighbor_val - base_val);

  // plan.indices.numel() (not the original row count!) is where the
  // retained/duplicated rows end and the first synthetic row begins --
  // the two only coincide when nothing fell back to plain duplication.
  const std::int64_t synthesized_row = plan.indices.numel();
  const double actual = balanced.features.x.slice(0, synthesized_row, synthesized_row + 1).item<double>();

  std::cout << "expected (base + step*(neighbor-base)) = " << expected << "\n";
  std::cout << "actual (from balance_rows output)      = " << actual << "\n";

  return 0;
}
