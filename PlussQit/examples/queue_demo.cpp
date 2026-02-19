#include <pluss/pluss.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

const char* kind_name(pluss::Queue::Kind k) {
  switch (k) {
    case pluss::Queue::Kind::Series: return "Series";
    case pluss::Queue::Kind::Note: return "Note";
    case pluss::Queue::Kind::Timeline: return "Timeline";
    case pluss::Queue::Kind::TimeIndex: return "TimeIndex";
  }
  return "Unknown";
}

} // namespace

int main() {
  std::cout << "=== pluss::Queue + dynamic dispense demo ===\n\n";

  pluss::Queue qs = pluss::value({-1.0, 2.0, -3.0});
  auto qs_abs = qs.invoke("abs");
  std::cout << "Queue kind after abs: " << kind_name(qs_abs.kind()) << "\n";
  const auto abs_vals = qs_abs.as<pluss::Series>().to_vec<double>();
  std::cout << "abs values: [" << abs_vals[0] << ", " << abs_vals[1] << ", " << abs_vals[2] << "]\n\n";

  pluss::Queue qn = pluss::content({"Alpha", "Beta"});
  auto qn_lower = qn.invoke("lower");
  std::cout << "Queue kind after lower: " << kind_name(qn_lower.kind()) << "\n";
  std::cout << "lower note[0]: " << qn_lower.as<pluss::Note>().at(0) << "\n\n";

  pluss::Queue qt = pluss::moment("2024-01-15T10:11:12Z");
  auto qt_year = qt.invoke("year");
  std::cout << "Queue Timeline->year kind: " << kind_name(qt_year.kind()) << "\n";
  std::cout << "year: " << qt_year.as<pluss::Series>().to_vec<std::int32_t>(0) << "\n\n";

  auto s = pluss::value({-4.0, 5.0, -6.0});
  auto s_abs = pluss::dispense<pluss::Series>(s, "abs");
  std::cout << "dispense<Series>(Series,\"abs\")[0]: " << s_abs.to_vec<double>(0) << "\n";

  auto n = pluss::content({"apple", "banana", "apricot"});
  auto starts_with_a = pluss::dispense<std::vector<std::uint8_t>>(n, "starts_with", "a");
  std::cout << "dispense<vector<uint8_t>>(Note,\"starts_with\",\"a\"): ["
            << static_cast<int>(starts_with_a[0]) << ", "
            << static_cast<int>(starts_with_a[1]) << ", "
            << static_cast<int>(starts_with_a[2]) << "]\n";

  auto t = pluss::moment("2024-01-15T10:11:12Z");
  auto day_floor = pluss::dispense<pluss::codeh::Timeline>(t, "floor", "D");
  std::cout << "dispense<Timeline>(Timeline,\"floor\",\"D\")->hour: "
            << day_floor.hour().to_vec<std::int32_t>(0) << "\n";

  auto any_year = pluss::dispense_any(t, "year");
  if (any_year.is<pluss::Series>()) {
    std::cout << "dispense_any(Timeline,\"year\") resolved to Series, value="
              << any_year.as<pluss::Series>().to_vec<std::int32_t>(0) << "\n";
  }

  std::cout << "\nDone.\n";
  return 0;
}
