#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// map() on a later frame, including an unseen category. Codes 1..K are the
// fitted categories; 0 is reserved for unseen/missing, so an unseen value
// writes the all-zero digit row.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train({{"code", Slate(pluss::make_series({10., 20., 30., 40., 50., 60., 70.}, {7}, f64))}});
  // 7 categories -> codes 1..7 -> ceil(log2(8)) = 3 binary digit columns.
  const DataFrame later({{"code", Slate(pluss::make_series({70., 10., 999.}, {3}, f64))}});

  lymph::prep::BaseNEncoder encoder;  // base = 2
  encoder.features({"code"}).learn(train);

  std::cout << "map(train) -- 7 categories in 3 binary digit columns:\n"
            << encoder.map(train) << "\n";

  lymph::InferOptions to_bucket;
  to_bucket.unknown_categorical = lymph::UnknownCategoryPolicy::MissingBucket;
  std::cout << "map(later, MissingBucket) -- unseen 999 -> code 0 -> all-zero row:\n"
            << encoder.map(later, to_bucket);

  return 0;
}
