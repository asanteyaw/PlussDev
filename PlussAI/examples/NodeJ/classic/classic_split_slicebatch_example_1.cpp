#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}, {5}, f64).reshape({5, 1});

  cls::SplitOptions options{};
  options.holdout_fraction = 0.4;
  const cls::IndexSplit split = cls::holdout_split(5, options).value();

  const cls::FeatureBatch train = cls::slice_feature_batch(batch, split.train_indices).value();
  const cls::FeatureBatch test = cls::slice_feature_batch(batch, split.test_indices).value();
  std::cout << "train.x = " << train.x << "\n";
  std::cout << "test.x  = " << test.x << "\n";

  return 0;
}
