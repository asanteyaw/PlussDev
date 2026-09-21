#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0}, {5}, f64).reshape({5, 1});

  cls::SplitOptions options{};
  options.holdout_fraction = 0.4;
  const cls::IndexSplit split = cls::holdout_split(batch.sample_count(), options).value();

  std::cout << "train_indices = " << split.train_indices << "\n";
  std::cout << "test_indices  = " << split.test_indices << "\n";

  const pluss::Series train_x = cls::slice_feature_batch(batch, split.train_indices).value().x;
  std::cout << "train x = " << train_x << "\n";

  return 0;
}
