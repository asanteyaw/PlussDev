// data::feature_batch_from_dataframe example 2 -- the Factorize fit/reuse
// round trip via existing_categories / fitted_categories_out.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame train_frame({
      {"income", Slate(pluss::make_series({1.0, 1.5, 2.0, 2.5}, {4}, f64))},
      {"region", Slate(pluss::make_note({"north", "south", "north", "east"}))},
  });

  cls::DataFrameBatchOptions options{};
  options.feature_names = {"income", "region"};
  options.note_policy = cls::DataNotePolicy::Factorize;

  cls::CategoryMaps fitted{};
  const cls::StatusOr<cls::FeatureBatch> train_batch =
      cls::feature_batch_from_dataframe(train_frame, options, /*existing_categories=*/nullptr, &fitted);
  std::cout << "train_batch.ok() = " << std::boolalpha << train_batch.ok() << "\n";
  std::cout << "fitted categories for 'region': ";
  for (const std::string& category : fitted.at("region").categories) std::cout << category << " ";
  std::cout << "\n";
  std::cout << "train x =\n" << train_batch.value().x << "\n";

  // A later, unrelated frame -- different row order, same category set.
  const DataFrame infer_frame({
      {"income", Slate(pluss::make_series({3.0, 1.8}, {2}, f64))},
      {"region", Slate(pluss::make_note({"south", "north"}))},
  });
  const cls::StatusOr<cls::FeatureBatch> infer_batch =
      cls::feature_batch_from_dataframe(infer_frame, options, &fitted, /*fitted_categories_out=*/nullptr);
  std::cout << "infer_batch.ok() = " << infer_batch.ok() << "\n";
  std::cout << "infer x =\n" << infer_batch.value().x << "\n";

  return 0;
}
