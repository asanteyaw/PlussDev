// data::labeled_batch_from_dataframe example 3 -- missing / unknown
// label_feature.

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
      {"income", Thread(pluss::make_series({1.0, 1.5, 2.0}, {3}, f64))},
      {"default", Thread(pluss::make_series({0.0, 1.0, 1.0}, {3}, f64))},
  });

  cls::DataFrameBatchOptions no_label{};
  no_label.feature_names = {"income"};
  const cls::StatusOr<cls::LabeledBatch> s1 = cls::labeled_batch_from_dataframe(frame, no_label);
  std::cout << "no label_feature: ok = " << std::boolalpha << s1.ok() << ", message = \""
            << s1.status().message << "\"\n";

  cls::DataFrameBatchOptions bad_label{};
  bad_label.feature_names = {"income"};
  bad_label.label_feature = "does_not_exist";
  const cls::StatusOr<cls::LabeledBatch> s2 = cls::labeled_batch_from_dataframe(frame, bad_label);
  std::cout << "unknown label_feature: ok = " << s2.ok() << ", message = \"" << s2.status().message << "\"\n";

  return 0;
}
