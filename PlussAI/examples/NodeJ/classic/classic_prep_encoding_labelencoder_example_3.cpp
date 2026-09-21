#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0}, {2}, f64).reshape({2, 1});
  cls::prep::LabelEncoder encoder;
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({30.0}, {1}, f64).reshape({1, 1});

  cls::InferOptions err_opts{};
  err_opts.unknown_categorical = cls::UnknownCategoryPolicy::Error;
  std::cout << "Error:         ok = " << std::boolalpha << encoder.map(probe, err_opts).ok() << "\n";

  cls::InferOptions bucket_opts{};
  bucket_opts.unknown_categorical = cls::UnknownCategoryPolicy::MissingBucket;
  std::cout << "MissingBucket: " << encoder.map(probe, bucket_opts).value() << "\n";

  return 0;
}
