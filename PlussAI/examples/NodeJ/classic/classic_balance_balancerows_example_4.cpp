#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 6 samples, 1 feature, Rows form, then transposed to Columns form.
  const pluss::Series row_x = pluss::make_series(
      {0.0, 10.0, 20.0, 30.0, 40.0, 500.0}, {6}, f64).reshape({6, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = row_x.transpose(0, 1).contiguous();
  batch.features.layout = cls::ObservationLayout::Columns;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Counts;
  target.counts = pluss::make_series({6.0, 2.0}, {2}, f64);
  target.growth = cls::BalanceGrowthKind::BlendNeighbors;
  target.neighbors = 2;
  cls::RandomState random{};
  random.seed = 23;
  const cls::LabeledBatch balanced = cls::balance_rows(batch, target, random).value();

  std::cout << "layout is Columns = " << std::boolalpha << (balanced.features.layout == cls::ObservationLayout::Columns) << "\n";
  std::cout << "sample_count() = " << balanced.features.sample_count() << "\n";

  return 0;
}
