#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({99.0}, {1}, f64).reshape({1, 1});
  cls::InferOptions bucket_opts{};
  bucket_opts.unknown_categorical = cls::UnknownCategoryPolicy::MissingBucket;

  cls::prep::OrdinalEncoder with_bucket;  // include_unknown_bucket = true (default)
  with_bucket.learn(train);
  std::cout << "include_unknown_bucket=true,  MissingBucket -> "
            << with_bucket.map(probe, bucket_opts).value() << "\n";

  cls::prep::OrdinalEncoderOptions no_bucket_opts{};
  no_bucket_opts.include_unknown_bucket = false;
  cls::prep::OrdinalEncoder without_bucket(no_bucket_opts);
  without_bucket.learn(train);
  std::cout << "include_unknown_bucket=false, MissingBucket -> "
            << without_bucket.map(probe, bucket_opts).value() << "\n";

  return 0;
}
