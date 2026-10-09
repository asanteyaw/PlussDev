#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Four categories with clearly different frequencies: 0 appears 4x, 1 3x,
  // 2 2x, 3 once.
  const pluss::Series tier = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 2, 2, 3}, {10}, f64);
  const DataFrame frame({{"tier", Slate(tier)}});

  // max_categories caps how many real category columns are produced; overflow
  // says what happens to the rest. KeepMostFrequent keeps the top-N by count
  // (0, 1, 2 here) and drops the tail (3).
  lymph::options::OneHotEncoder capped{};
  capped.max_categories = 3;
  capped.overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent;
  capped.include_unknown_bucket = true;  // reserve a column for dropped/unseen

  // At map time, route anything not in the kept set into that reserved bucket
  // rather than raising.
  lymph::options::Infer infer{};
  infer.unknown_categorical = lymph::UnknownCategoryPolicy::MissingBucket;

  lymph::prep::OneHotEncoder encoder{capped};
  encoder.features({"tier"});
  const DataFrame expanded = encoder.learn_map(frame, {}, infer);

  std::cout << "4 categories, max_categories = 3  ->  " << expanded.width()
            << " columns (3 kept + 1 bucket)\n\n";
  std::cout << expanded;

  return 0;
}
