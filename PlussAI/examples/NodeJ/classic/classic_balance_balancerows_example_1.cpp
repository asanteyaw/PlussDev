#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 100.0, 200.0}, {8}, f64).reshape({8, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Equalize;
  cls::RandomState random{};
  random.seed = 13;
  const cls::LabeledBatch balanced = cls::balance_rows(batch, target, random).value();
  std::cout << "sample_count() = " << balanced.features.sample_count() << "\n";
  std::cout << "class_mix.counts = " << cls::class_mix(balanced.y).value().counts << "\n";

  return 0;
}
