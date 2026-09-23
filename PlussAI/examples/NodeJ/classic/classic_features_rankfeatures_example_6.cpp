#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({2, 2});

  cls::LabeledBatch nan_labels{};
  nan_labels.features.x = x;
  nan_labels.y = pluss::make_series({0.0, nan}, {2}, f64);
  std::cout << "NaN label ok           = " << std::boolalpha << cls::rank_features(nan_labels).ok() << "\n";

  cls::LabeledBatch single_class{};
  single_class.features.x = x;
  single_class.y = pluss::make_series({1.0, 1.0}, {2}, f64);
  std::cout << "single-class ClassF ok = " << cls::rank_features(single_class).ok() << "\n";

  const cls::Series small_x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({1, 2});
  cls::LabeledBatch too_few{};
  too_few.features.x = small_x;
  too_few.y = pluss::make_series({5.0}, {1}, f64);
  cls::FeatureRankOptions regression{};
  regression.mode = cls::FeatureScoreMode::RegressionF;
  std::cout << "1 sample RegressionF ok = " << cls::rank_features(too_few, regression).ok() << "\n";

  cls::LabeledBatch constant_target{};
  constant_target.features.x = x;
  constant_target.y = pluss::make_series({5.0, 5.0}, {2}, f64);
  std::cout << "constant target RegressionF ok = " << cls::rank_features(constant_target, regression).ok() << "\n";

  return 0;
}
