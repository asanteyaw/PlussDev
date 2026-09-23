#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Config i64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {0.0, 10.0, 20.0, 30.0, 40.0, 500.0}, {6}, f64).reshape({6, 1});
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1}, {6}, i64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Counts;
  target.counts = pluss::make_series({6.0, 2.0}, {2}, f64);
  target.growth = cls::BalanceGrowthKind::BlendNeighbors;
  target.neighbors = 2;
  const cls::LabeledBatch balanced = cls::balance_rows(batch, target).value();

  std::cout << "batch.y dtype is Int64     = " << std::boolalpha << (batch.y.scalar_type() == pluss::ScalarType::Int64) << "\n";
  std::cout << "balanced.y dtype is Int64  = " << (balanced.y.scalar_type() == pluss::ScalarType::Int64) << "\n";

  return 0;
}
