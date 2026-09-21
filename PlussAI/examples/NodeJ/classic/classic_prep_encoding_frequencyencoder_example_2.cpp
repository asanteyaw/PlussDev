#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 1.0}, {3}, f64).reshape({3, 1});
  cls::prep::FrequencyEncoder encoder;
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({9.0}, {1}, f64).reshape({1, 1});

  cls::InferOptions err_opts{};
  err_opts.unknown_categorical = cls::UnknownCategoryPolicy::Error;
  std::cout << "Error:         ok = " << std::boolalpha << encoder.map(probe, err_opts).ok() << "\n";

  cls::InferOptions bucket_opts{};
  bucket_opts.unknown_categorical = cls::UnknownCategoryPolicy::MissingBucket;
  std::cout << "MissingBucket: " << encoder.map(probe, bucket_opts).value() << "\n";

  cls::InferOptions ignore_opts{};
  ignore_opts.unknown_categorical = cls::UnknownCategoryPolicy::Ignore;
  std::cout << "Ignore:        " << encoder.map(probe, ignore_opts).value() << "\n";

  return 0;
}
