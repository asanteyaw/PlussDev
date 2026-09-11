// data::FeatureBatch example 2 -- validate_features on an undefined batch and
// a rank-1 batch.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::FeatureBatch undefined{};
  const cls::Status s1 = cls::validate_features(undefined);
  std::cout << "undefined: ok = " << std::boolalpha << s1.ok() << ", message = \"" << s1.message << "\"\n";

  cls::FeatureBatch rank1{};
  rank1.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64);  // missing the feature axis
  const cls::Status s2 = cls::validate_features(rank1);
  std::cout << "rank1:     ok = " << s2.ok() << ", message = \"" << s2.message << "\"\n";

  cls::FeatureBatch valid{};
  valid.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  const cls::Status s3 = cls::validate_features(valid);
  std::cout << "valid:     ok = " << s3.ok() << "\n";

  return 0;
}
