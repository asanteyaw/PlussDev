// data::FeatureBatch example 3 -- built from a DataFrame instead of by hand.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Slate(pluss::make_series({1.0, 1.5, 2.0, 2.5}, {4}, f64))},
      {"balance", Slate(pluss::make_series({0.4, 0.6, 0.8, 1.0}, {4}, f64))},
  });

  cls::DataFrameBatchOptions options{};
  options.feature_names = {"income", "balance"};

  const cls::StatusOr<cls::FeatureBatch> made = cls::feature_batch_from_dataframe(frame, options);
  std::cout << "made.ok() = " << std::boolalpha << made.ok() << "\n";
  std::cout << "sample_count()  = " << made.value().sample_count() << "\n";
  std::cout << "feature_count() = " << made.value().feature_count() << "\n";

  return 0;
}
