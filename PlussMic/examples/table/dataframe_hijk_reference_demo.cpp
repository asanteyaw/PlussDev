#include <iostream>
#include <optional>
#include <sstream>

#include <pluss/pluss.h>

int main() {
  using pluss::codem::ScalarType;
  using pluss::kindle::KindelizePolicy;
  using pluss::make_note;
  using pluss::make_series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  auto sales = DataFrame({
      {"city", make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
      {"region", make_note({"south", "south", "middle", "north"})},
      {"units", make_series({10.0, 7.0, 12.0, 5.0})},
      {"price", make_series({3.0, 4.0, 2.5, 5.0})},
  }).title("sales");

  auto valid = pluss::Series::zeros({4}, ScalarType::Bool);
  auto* valid_ptr = reinterpret_cast<bool*>(valid.raw_data());
  valid_ptr[0] = true;
  valid_ptr[1] = false;
  valid_ptr[2] = true;
  valid_ptr[3] = false;

  auto sales_missing = DataFrame({
      {"city", make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
      {"region", Thread(make_note({"south", "", "middle", ""}), valid)},
      {"units", Thread(make_series({10.0, 0.0, 12.0, 0.0}), valid)},
      {"price", make_series({3.0, 4.0, 2.5, 5.0})},
  }).title("sales_missing");

  auto metadata = DataFrame({
      {"region", make_note({"capital", "harbor", "garden", "savannah"})},
      {"inventory", make_series({100.0, 80.0, 65.0, 40.0})},
  }).title("metadata");

  std::cout << "SALES\n" << sales.to_string() << "\n\n";
  std::cout << "SALES_MISSING\n" << sales_missing.to_string() << "\n\n";

  std::cout << "HAS_FEATURE\n";
  std::cout << "units: " << (sales.get("units").has_value() ? "true" : "false") << "\n";
  std::cout << "inventory: " << (sales.get("inventory").has_value() ? "true" : "false") << "\n\n";

  std::cout << "HEAD\n" << sales.head(2, false).to_string() << "\n\n";
  std::cout << "HEIGHT\n" << sales.height() << "\n\n";
  std::cout << "IAT\n";
  std::cout << "sales.case_study().at(1).features({\"score\"}):\n"
            << sales.case_study().at(1).features({sales.features()[3]}).to_string() << "\n";
  std::cout << "sales.case_study().at(2).features({\"student\"}):\n"
            << sales.case_study().at(2).features({sales.features()[0]}).to_string() << "\n\n";

  std::cout << "INFO_STRING\n" << sales.info_string() << "\n\n";

  std::ostringstream info_os;
  sales.info(info_os);
  std::cout << "INFO_OSTREAM\n" << info_os.str() << "\n\n";

  std::cout << "INSERT\n";
  std::cout << sales.insert(3, "tip", make_series({0.45, 0.35, 0.55, 0.25})).to_string() << "\n\n";

  std::cout << "ISNA_ALL\n" << sales_missing.isnull().to_string() << "\n\n";
  std::cout << "ISNULL_SELECTED\n" << sales_missing.isnull({"region", "units"}).to_string() << "\n\n";

  std::cout << "JOIN_DEFAULT\n" << sales.join(metadata).to_string() << "\n\n";
  std::cout << "JOIN_CUSTOM_SUFFIX\n" << sales.join(metadata, "_meta").to_string() << "\n\n";

  const auto catalog = sales.kindelize();
  std::cout << "KINDELIZE_KEYS\n";
  for (const auto& key : catalog.keys()) {
    std::cout << key << "\n";
  }
  std::cout << "\nKINDELIZE_STREAM\n" << catalog.keys("city").head(2).to_string() << "\n\n";

  KindelizePolicy policy;
  policy.materialize_label = true;
  policy.label_key = "row_id";
  const auto catalog_with_cases = sales.kindelize(policy);
  std::cout << "KINDELIZE_WITH_POLICY_KEYS\n";
  for (const auto& key : catalog_with_cases.keys()) {
    std::cout << key << "\n";
  }
  std::cout << "\nKINDELIZE_WITH_POLICY_STREAM\n"
            << catalog_with_cases.keys("row_id").head(3).to_string() << "\n";

  return 0;
}
