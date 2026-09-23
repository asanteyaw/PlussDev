#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);

  cls::BalanceTarget wants_clean{};
  wants_clean.clean = cls::BalanceCleanKind::BoundaryPairs;
  std::cout << "Series-only overload + clean set ok = " << std::boolalpha << cls::plan_balance(labels, wants_clean).ok() << "\n";

  cls::BalanceTarget wants_blend{};
  wants_blend.growth = cls::BalanceGrowthKind::BlendNeighbors;
  std::cout << "Series-only overload + BlendNeighbors ok = " << cls::plan_balance(labels, wants_blend).ok() << "\n";

  const pluss::Series three_classes = pluss::make_series({0.0, 1.0, 2.0}, {3}, f64);
  cls::BalanceTarget ratio_target{};
  ratio_target.kind = cls::BalanceTargetKind::Ratio;
  std::cout << "Ratio with 3 classes ok = " << cls::plan_balance(three_classes, ratio_target).ok() << "\n";

  cls::BalanceTarget bad_counts{};
  bad_counts.kind = cls::BalanceTargetKind::Counts;
  bad_counts.counts = pluss::make_series({0.0, 4.0}, {2}, f64);  // 0 is < 1
  std::cout << "Counts with a value < 1 ok = " << cls::plan_balance(labels, bad_counts).ok() << "\n";

  const pluss::Series x = pluss::make_series({0.0, 1.0, 2.0, 3.0}, {4}, f64).reshape({4, 1});
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = labels;
  cls::BalanceTarget bad_neighbors{};
  bad_neighbors.growth = cls::BalanceGrowthKind::BlendNeighbors;
  bad_neighbors.neighbors = 0;
  std::cout << "BlendNeighbors with neighbors=0 ok = " << cls::plan_balance(batch, bad_neighbors).ok() << "\n";

  return 0;
}
