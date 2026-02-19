#include <pluss/pluss.h>

#include <any>
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
  auto s_numel = pluss::dispense_as<std::int64_t>(
      s, &pluss::Series::numel, pluss::args{});
  std::cout << "dispense(Series,&Series::numel,args{}): " << s_numel << "\n";

  auto n = pluss::content({"apple", "banana", "apricot"});
  pluss::args starts_with_args;
  starts_with_args.emplace_back(std::string_view("a"));
  auto starts_with_a = pluss::dispense_as<std::vector<std::uint8_t>>(
      n, &pluss::Note::starts_with, starts_with_args);
  std::cout << "dispense(Note,&Note::starts_with,args{...}): ["
            << static_cast<int>(starts_with_a[0]) << ", "
            << static_cast<int>(starts_with_a[1]) << ", "
            << static_cast<int>(starts_with_a[2]) << "]\n";

  auto t = pluss::moment("2024-01-15T10:11:12Z");
  pluss::args floor_args;
  floor_args.emplace_back(std::string_view("D"));
  auto day_floor = pluss::dispense_as<pluss::codeh::Timeline>(
      t, &pluss::codeh::Timeline::floor, floor_args);
  std::cout << "dispense(Timeline,&Timeline::floor,args{...})->hour: "
            << day_floor.hour().to_vec<std::int32_t>(0) << "\n";

  auto year_series = pluss::dispense_as<pluss::Series>(
      t, &pluss::codeh::Timeline::year, pluss::args{});
  std::cout << "dispense(Timeline,&Timeline::year,args{}) value="
            << year_series.to_vec<std::int32_t>(0) << "\n";

  std::cout << "\nDone.\n";
  return 0;
}
