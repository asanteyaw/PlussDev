#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <any>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
  std::cout << "=== pluss::Queue + dynamic dispense demo ===\n\n";

  // Keep demo portable across machines without MPS runtime.
  pluss::Queue qs = pluss::make_series({-1.0, 2.0, -3.0});
  std::cout << "Queue: " << qs << "\n";
  std::cout << "Queue.as_series()[1]: " << qs.as_series().to_vec<double>()[1] << "\n";
  std::cout << "Queue.series().log()[0]: " << qs.series().log().to_vec<double>()[0] << "\n";
  const auto qs_chain = qs.invoke({"abs", "log", "mean"});
  const auto qs_b = qs.series().abs().log().mean();
  const pluss::Queue qk = qs.series().abs().log().mean(); // if you want the chain result as a Queue directly
  std::cout << "Queue.invoke({\"abs\",\"log\",\"mean\"}): " << qs_chain << "\n";
  std::cout << "Queue.invoke({\"abs\",\"log\",\"mean\"}) kind: " << qs_chain.kind_name() << "\n";
  std::cout << "Queue.series().abs().log().mean():\n" << qs_b << "\n";
  std::cout << "Queue level Queue.series().abs().log().mean():\n" << qk << "\n";
  std::cout << "Queue binary +: " << (qs + pluss::Queue(pluss::make_series({1.0, 1.0, 1.0}))) << "\n";
  std::cout << "Queue binary >: " << (qs > pluss::Queue(pluss::make_series({0.0, 0.0, 0.0}))) << "\n";
  auto qs_abs = qs.invoke("abs");
  let qs_itm = qs_b.item<double>();
  let qs_item = qs_b.item(); // if you want to dispense the single scalar result directly
  std::cout << "Series after explicit item: " << qs_itm << "\n";
  std::cout << "Dispensed item: " << qs_item << "\n";
  std::cout << "Dispensed item dtype: " << qs_b.item().dtype() << "\n";
  std::cout << "Queue kind after abs: " << qs_abs.kind_name() << "\n";
  const auto abs_vals = qs_abs.as<pluss::Series>().to_vec<double>();
  std::cout << "abs values: [" << abs_vals[0] << ", " << abs_vals[1] << ", " << abs_vals[2] << "]\n\n";

  pluss::Queue qn = pluss::make_note({"Alpha", "Beta"});
  std::cout << "Queue.note().lower()[0]: " << qn.note().lower().at(0) << "\n";
  std::cout << "Queue Note + Note: " << (qn + pluss::Queue(pluss::make_note({"_x", "_y"}))) << "\n";
  std::cout << "Queue Note == Note: " << (qn == pluss::Queue(pluss::make_note({"Alpha", "Zeta"}))) << "\n";
  auto qn_lower = qn.invoke("lower");
  std::cout << "Queue kind after lower: " << qn_lower.kind_name() << "\n";
  std::cout << "lower note[0]: " << qn_lower.as<pluss::Note>().at(0) << "\n\n";

  pluss::Queue qt = pluss::make_timeline("2024-01-15T10:11:12Z");
  std::cout << "Queue.timeline().hour(): " << qt.timeline().hour().to_vec<std::int32_t>(0) << "\n";
  auto qt_year = qt.invoke("year");
  std::cout << "Queue Timeline->year kind: " << qt_year.kind_name() << "\n";
  std::cout << "year: " << qt_year.as<pluss::Series>().to_vec<std::int32_t>(0) << "\n\n";

  pluss::Queue qt1d = pluss::make_timeline({
      "2024-01-15T10:11:12Z",
      "2024-01-16T10:11:12Z",
  });
  auto qidx = qt1d.invoke("to_timeindex", {true});
  std::cout << "Queue.as_timeindex().size(): " << qidx.as_timeindex().size() << "\n\n";

  auto mask = pluss::make_series({true, false, true});
  auto lhs = pluss::make_series({10.0, 20.0, 30.0});
  auto rhs = pluss::make_series({1.0, 2.0, 3.0});
  std::cout << "Queue::where(mask, lhs, rhs): "
            << pluss::Queue::where(pluss::Queue(mask), pluss::Queue(lhs), pluss::Queue(rhs))
            << "\n\n";

  auto s = pluss::make_series({-4.0, 5.0, -6.0});
  auto s_numel = pluss::dispense_as<std::int64_t>(
      s, &pluss::Series::numel, pluss::args{});
  std::cout << "dispense(Series,&Series::numel,args{}): " << s_numel << "\n";

  auto n = pluss::make_note({"apple", "banana", "apricot"});
  pluss::args starts_with_args;
  starts_with_args.emplace_back(std::string_view("a"));
  auto starts_with_a = pluss::dispense_as<std::vector<std::uint8_t>>(
      n, &pluss::Note::starts_with, starts_with_args);
  std::cout << "dispense(Note,&Note::starts_with,args{...}): ["
            << static_cast<int>(starts_with_a[0]) << ", "
            << static_cast<int>(starts_with_a[1]) << ", "
            << static_cast<int>(starts_with_a[2]) << "]\n";

  auto t = pluss::make_timeline("2024-01-15T10:11:12Z");
  pluss::args floor_args;
  floor_args.emplace_back(std::string_view("D"));
  auto day_floor = pluss::dispense_as<pluss::Timeline>(
      t, &pluss::Timeline::floor, floor_args);
  std::cout << "dispense(Timeline,&Timeline::floor,args{...})->hour: "
            << day_floor.hour().to_vec<std::int32_t>(0) << "\n";

  auto year_series = pluss::dispense_as<pluss::Series>(
      t, &pluss::Timeline::year, pluss::args{});
  std::cout << "dispense(Timeline,&Timeline::year,args{}) value="
            << year_series.to_vec<std::int32_t>(0) << "\n";

  std::cout << "\nDone.\n";
  return 0;
}


// Architecture and subsystem boundaries: autodiff_architecture.md
// Practical contract for gradient-based optimization implementers: optimization_diff_contract.md
