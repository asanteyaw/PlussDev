#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame train({{"region", Thread(pluss::make_series({0., 1., 2., 0., 1., 2.}, {6}, f64))}});
  // Row 1 carries category 9, never seen at fit time.
  const DataFrame later({{"region", Thread(pluss::make_series({1., 9.}, {2}, f64))}});

  lymph::prep::OneHotEncoder encoder;
  encoder.features({"region"}).learn(train);

  // Fitting reserves a region_unknown column (include_unknown_bucket = true).
  // What map() does with an unknown value is a separate, inference-time choice
  // carried on InferOptions::unknown_categorical -- and it defaults to Error.
  try {
    const DataFrame out = encoder.map(later);
    std::cout << "default: " << out.ncases() << " rows\n";
  } catch (const std::exception& ex) {
    std::cout << "default (Error) threw: " << ex.what() << "\n\n";
  }

  lymph::InferOptions to_bucket;
  to_bucket.unknown_categorical = lymph::UnknownCategoryPolicy::MissingBucket;
  std::cout << "MissingBucket -- unknown row lands in region_unknown:\n"
            << encoder.map(later, to_bucket) << "\n";

  lymph::InferOptions ignore;
  ignore.unknown_categorical = lymph::UnknownCategoryPolicy::Ignore;
  std::cout << "Ignore -- unknown row is left all-zero:\n"
            << encoder.map(later, ignore);

  return 0;
}
