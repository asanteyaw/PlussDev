// data::feature_batch_from_dataframe example 3 -- an unseen category at reuse
// time under each of the three factorize_unknown_categorical policies.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <utility>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame train_frame({
      {"income", Slate(pluss::make_series({1.0, 1.5, 2.0}, {3}, f64))},
      {"region", Slate(pluss::make_note({"north", "south", "north"}))},
  });

  cls::DataFrameBatchOptions options{};
  options.feature_names = {"income", "region"};
  options.note_policy = cls::DataNotePolicy::Factorize;

  cls::CategoryMaps fitted{};
  cls::feature_batch_from_dataframe(train_frame, options, nullptr, &fitted);

  // "west" never appeared during training.
  const DataFrame surprise_frame({
      {"income", Slate(pluss::make_series({2.2}, {1}, f64))},
      {"region", Slate(pluss::make_note({"west"}))},
  });

  for (auto [name, policy] : {std::pair{"Error", cls::UnknownCategoryPolicy::Error},
                              std::pair{"MissingBucket", cls::UnknownCategoryPolicy::MissingBucket},
                              std::pair{"Ignore", cls::UnknownCategoryPolicy::Ignore}}) {
    cls::DataFrameBatchOptions probe = options;
    probe.factorize_unknown_categorical = policy;
    const cls::StatusOr<cls::FeatureBatch> result =
        cls::feature_batch_from_dataframe(surprise_frame, probe, &fitted, nullptr);
    std::cout << name << ": ok = " << std::boolalpha << result.ok();
    if (!result.ok()) {
      std::cout << ", message = \"" << result.status().message << "\"";
    } else {
      std::cout << ", region code = " << result.value().x;
    }
    std::cout << "\n";
  }

  return 0;
}
