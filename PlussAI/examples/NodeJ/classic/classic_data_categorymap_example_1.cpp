// data::CategoryMap example 1 -- reading a fitted map back, and the reserved
// unknown-category code.

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
      {"region", Slate(pluss::make_note({"north", "south", "east", "south"}))},
  });

  cls::DataFrameBatchOptions options{};
  options.feature_names = {"income", "region"};
  options.note_policy = cls::DataNotePolicy::Factorize;

  cls::CategoryMaps fitted{};
  cls::feature_batch_from_dataframe(frame, options, nullptr, &fitted);

  const cls::CategoryMap& region_map = fitted.at("region");
  std::cout << "region categories, code -> string:\n";
  for (std::size_t code = 0; code < region_map.categories.size(); ++code) {
    std::cout << "  " << code << " -> " << region_map.categories[code] << "\n";
  }
  std::cout << "reserved unknown-category code = " << region_map.categories.size() << "\n";
  std::cout << "'region' present in fitted map = " << std::boolalpha
            << (fitted.find("region") != fitted.end()) << "\n";
  std::cout << "'income' present in fitted map = " << (fitted.find("income") != fitted.end())
            << "  (only Factorize columns are captured)\n";

  return 0;
}
