#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);

  const pluss::Series wrong_length = pluss::make_series({1.0, 1.0}, {2}, f64);
  std::cout << "length-mismatched batch_sample_weights ok = "
            << std::boolalpha << cls::resolve_weight_plan(labels, wrong_length, cls::TrainOptions{}).ok() << "\n";

  cls::TrainOptions zero_weight{};
  zero_weight.sample_weights = pluss::make_series({1.0, 0.0, 1.0, 1.0}, {4}, f64);
  std::cout << "non-positive options.sample_weights ok = "
            << cls::resolve_weight_plan(labels, pluss::Series::undefined(), zero_weight).ok() << "\n";

  cls::TrainOptions missing_custom{};
  missing_custom.class_weight = cls::ClassWeightMode::Custom;
  std::cout << "Custom with class_weights undefined ok = "
            << cls::resolve_weight_plan(labels, pluss::Series::undefined(), missing_custom).ok() << "\n";

  cls::TrainOptions wrong_custom_length{};
  wrong_custom_length.class_weight = cls::ClassWeightMode::Custom;
  wrong_custom_length.class_weights = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64);
  std::cout << "Custom class_weights length mismatch ok = "
            << cls::resolve_weight_plan(labels, pluss::Series::undefined(), wrong_custom_length).ok() << "\n";

  return 0;
}
