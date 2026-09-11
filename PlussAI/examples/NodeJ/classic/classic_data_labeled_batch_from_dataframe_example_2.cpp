// data::labeled_batch_from_dataframe example 2 -- sample_weight_feature
// resolution.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::DataFrame;
  using pluss::table::Thread;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Thread(pluss::make_series({1.0, 1.5, 2.0, 2.5}, {4}, f64))},
      {"default", Thread(pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64))},
      {"importance", Thread(pluss::make_series({1.0, 5.0, 1.0, 1.0}, {4}, f64))},
  });

  cls::DataFrameBatchOptions options{};
  options.feature_names = {"income"};
  options.label_feature = "default";
  options.sample_weight_feature = "importance";

  const cls::StatusOr<cls::LabeledBatch> made = cls::labeled_batch_from_dataframe(frame, options);
  std::cout << "has_weights() = " << std::boolalpha << made.value().has_weights() << "\n";
  std::cout << "sample_weights = " << made.value().sample_weights << "\n";

  return 0;
}
