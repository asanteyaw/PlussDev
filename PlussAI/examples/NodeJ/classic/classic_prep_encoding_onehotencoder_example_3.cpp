#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});

  cls::prep::OneHotEncoderOptions options{};
  options.include_unknown_bucket = false;
  cls::prep::OneHotEncoder encoder(options);
  encoder.learn(train);

  cls::FeatureBatch known{};
  known.x = pluss::make_series({2.0}, {1}, f64).reshape({1, 1});
  std::cout << "known value 2   -> " << encoder.map(known).value() << "\n";

  cls::FeatureBatch unknown{};
  unknown.x = pluss::make_series({9.0}, {1}, f64).reshape({1, 1});
  cls::InferOptions bucket_opts{};
  bucket_opts.unknown_categorical = cls::UnknownCategoryPolicy::MissingBucket;
  std::cout << "unknown (no bucket to route to) -> " << encoder.map(unknown, bucket_opts).value() << "\n";

  return 0;
}
