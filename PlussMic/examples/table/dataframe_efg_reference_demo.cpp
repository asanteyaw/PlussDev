#include <iostream>
#include <optional>

#include <pluss/pluss.h>

int main() {
  using pluss::codem::ScalarType;
  using pluss::make_note;
  using pluss::make_series;
  using pluss::make_timeline;
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

  auto events = DataFrame({
      {"event", make_note({"open", "midday", "close"})},
      {"stamp", make_timeline({
          "2024-01-01T09:10:00Z",
          "2024-01-01T11:45:00Z",
          "2024-01-02T15:05:00Z",
      })},
      {"load", make_series({10.0, 14.0, 9.0})},
  }).title("events");

  std::cout << "SALES\n" << sales.to_string() << "\n\n";
  std::cout << "SALES_MISSING\n" << sales_missing.to_string() << "\n\n";
  std::cout << "FEATURE_NAMES\n";
  for (const auto& name : sales.features()) {
    std::cout << name << "\n";
  }
  std::cout << "FEATURE_SINGLE\n" << sales.features("units").to_string() << "\n\n";
  std::cout << "FEATURE_MULTI\n" << sales.features({"city", "units"}).to_string() << "\n\n";

  std::cout << "GET_EXISTING\n" << (sales.get("units").has_value() ? "has_value" : "none") << "\n";
  std::cout << "GET_MISSING\n" << (sales.get("inventory").has_value() ? "has_value" : "none") << "\n\n";

  std::cout << "FILTER\n" << sales.filter(sales.features("region") == "south").to_string() << "\n\n";

  std::cout << "FILLNA_NUMERIC\n" << sales_missing.fillnull(0.0, {"units"}).to_string() << "\n\n";
  std::cout << "FILLNA_NOTE\n" << sales_missing.fillnull("unknown", {"region"}).to_string() << "\n\n";

  auto sales_mut = sales_missing.copy();
  sales_mut.fillnull_(0.0);
  std::cout << "FILLNA_INPLACE\n" << sales_mut.to_string() << "\n\n";

  std::cout << "FFILL\n" << sales_missing.ffill().to_string() << "\n\n";
  std::cout << "FLOORDIV\n" << sales.features({"units", "price"}).floordiv(2.0).to_string() << "\n\n";
  std::cout << "GE\n" << sales.features({"units", "price"}).ge(5.0).to_string() << "\n\n";
  std::cout << "GT\n" << sales.features({"units", "price"}).gt(5.0).to_string() << "\n\n";

  std::cout << "GROUP_BY\n";
  std::cout << sales.group_by(
      {"region"},
      {
          {"total_units", "units", DataFrame::GroupOp::Sum},
          {"avg_price", "price", DataFrame::GroupOp::Mean},
      },
      true,
      false).to_string() << "\n\n";

  std::cout << "EVENTS\n" << events.to_string() << "\n\n";
  std::cout << "FLOOR_TIME\n" << events.tl("stamp").floor("day").to_string() << "\n";
  return 0;
}
