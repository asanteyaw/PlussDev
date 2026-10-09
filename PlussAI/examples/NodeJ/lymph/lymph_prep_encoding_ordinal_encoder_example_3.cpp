#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// An unseen category at map time. include_unknown_bucket reserves a slot for it;
// what map() does when it meets one is InferOptions::unknown_categorical, which
// defaults to Error. MissingBucket routes it to a reserved code just past the
// fitted range (here the 3 fitted categories are 1..3, so unseen -> 4).
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train({{"region", Slate(pluss::make_series({10., 20., 30., 10., 20., 30.}, {6}, f64))}});
  // Row 2 carries category 99, never seen at fit time.
  const DataFrame later({{"region", Slate(pluss::make_series({30., 10., 99.}, {3}, f64))}});

  lymph::prep::OrdinalEncoder encoder;
  encoder.features({"region"}).learn(train);

  try {
    (void)encoder.map(later);
    std::cout << "default: no throw\n";
  } catch (const std::exception& ex) {
    std::cout << "default (Error) threw: " << ex.what() << "\n\n";
  }

  lymph::InferOptions to_bucket;
  to_bucket.unknown_categorical = lymph::UnknownCategoryPolicy::MissingBucket;
  std::cout << "MissingBucket -- unseen 99 maps to the reserved code 4:\n"
            << encoder.map(later, to_bucket);

  return 0;
}
