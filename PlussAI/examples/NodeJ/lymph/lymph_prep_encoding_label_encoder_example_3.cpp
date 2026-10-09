#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// learn_map in one call, and the shared unknown-category handling: with the
// default Error policy an unseen value at map time fails; MissingBucket routes
// it to the reserved code just past the fitted range (3 fitted categories are
// 0..2, so unseen -> 3).
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train({{"grade", Slate(pluss::make_series({10., 20., 30., 20., 10., 30.}, {6}, f64))}});
  const DataFrame later({{"grade", Slate(pluss::make_series({20., 77.}, {2}, f64))}});

  lymph::prep::LabelEncoder encoder;
  std::cout << "learn_map(train):\n" << encoder.features({"grade"}).learn_map(train) << "\n";

  lymph::InferOptions to_bucket;
  to_bucket.unknown_categorical = lymph::UnknownCategoryPolicy::MissingBucket;
  std::cout << "map(later, MissingBucket) -- unseen 77 -> reserved code 3:\n"
            << encoder.map(later, to_bucket);

  return 0;
}
