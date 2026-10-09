// data::labeled_batch_from_dataframe example 1 -- features plus a label
// column, no weights.

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
      {"default", Slate(pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64))},
  });

  cls::DataFrameBatchOptions options{};
  options.feature_names = {"income", "balance"};
  options.label_feature = "default";

  const cls::StatusOr<cls::LabeledBatch> made = cls::labeled_batch_from_dataframe(frame, options);
  std::cout << "made.ok() = " << std::boolalpha << made.ok() << "\n";
  std::cout << "has_labels()  = " << made.value().has_labels() << "\n";
  std::cout << "has_weights() = " << made.value().has_weights() << "\n";
  std::cout << "y = " << made.value().y << "\n";

  return 0;
}
