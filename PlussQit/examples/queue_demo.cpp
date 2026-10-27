#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <any>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
  std::cout << "=== pluss::Type + dynamic dispense demo ===\n\n";

  // Keep demo portable across machines without MPS runtime.
  pluss::Type qs = pluss::make_series({-1.0, 2.0, -3.0});
  std::cout << "Type: " << qs << "\n";
  std::cout << "Type.as_series()[1]: " << qs.as_series().host<double>()[1] << "\n";
  std::cout << "Type.series().log()[0]: " << qs.series().log().host<double>()[0] << "\n";
  const auto qs_chain = qs.invoke({"abs", "log", "mean"});
  const auto qs_b = qs.series().abs().log().mean();
  const pluss::Type qk = qs.series().abs().log().mean(); // if you want the chain result as a Type directly
  std::cout << "Type.invoke({\"abs\",\"log\",\"mean\"}): " << qs_chain << "\n";
  std::cout << "Type.invoke({\"abs\",\"log\",\"mean\"}) kind: " << qs_chain.kind_name() << "\n";
  std::cout << "Type.series().abs().log().mean():\n" << qs_b << "\n";
  std::cout << "Type level Type.series().abs().log().mean():\n" << qk << "\n";
  std::cout << "Type binary +: " << (qs + pluss::Type(pluss::make_series({1.0, 1.0, 1.0}))) << "\n";
  std::cout << "Type binary >: " << (qs > pluss::Type(pluss::make_series({0.0, 0.0, 0.0}))) << "\n";
  auto qs_abs = qs.invoke("abs");
  auto qs_itm = qs_b.item<double>();
  auto qs_item = qs_b.item(); // if you want to dispense the single scalar result directly
  std::cout << "Series after explicit item: " << qs_itm << "\n";
  std::cout << "Dispensed item: " << qs_item << "\n";
  std::cout << "Dispensed item dtype: " << qs_b.item().dtype() << "\n";
  std::cout << "Type kind after abs: " << qs_abs.kind_name() << "\n";
  const auto abs_vals = qs_abs.as<pluss::Series>().host<double>();
  std::cout << "abs values: [" << abs_vals[0] << ", " << abs_vals[1] << ", " << abs_vals[2] << "]\n\n";

  pluss::Type qn = pluss::make_note({"Alpha", "Beta"});
  std::cout << "Type.note().lower()[0]: " << qn.note().lower().at(0) << "\n";
  std::cout << "Type Note + Note: " << (qn + pluss::Type(pluss::make_note({"_x", "_y"}))) << "\n";
  std::cout << "Type Note == Note: " << (qn == pluss::Type(pluss::make_note({"Alpha", "Zeta"}))) << "\n";
  auto qn_lower = qn.invoke("lower");
  std::cout << "Type kind after lower: " << qn_lower.kind_name() << "\n";
  std::cout << "lower note[0]: " << qn_lower.as<pluss::Note>().at(0) << "\n\n";

  pluss::Type qt = pluss::timeline::make("2024-01-15T10:11:12Z");
  std::cout << "Type.timeline().hour(): " << qt.timeline().hour().host<std::int32_t>().array().data.at(static_cast<std::size_t>(0)) << "\n";
  auto qt_year = qt.invoke("year");
  std::cout << "Type Timeline->year kind: " << qt_year.kind_name() << "\n";
  std::cout << "year: " << qt_year.as<pluss::Series>().host<std::int32_t>().array().data.at(static_cast<std::size_t>(0)) << "\n\n";

  pluss::Type qt1d = pluss::timeline::make({
      "2024-01-15T10:11:12Z",
      "2024-01-16T10:11:12Z",
  });
  auto qidx = qt1d.timeline().index();
  std::cout << "Type.timeline().index().size(): " << qidx.size() << "\n\n";

  auto mask = pluss::make_series({true, false, true});
  auto lhs = pluss::make_series({10.0, 20.0, 30.0});
  auto rhs = pluss::make_series({1.0, 2.0, 3.0});
  std::cout << "Type::where(mask, lhs, rhs): "
            << pluss::Type::where(pluss::Type(mask), pluss::Type(lhs), pluss::Type(rhs))
            << "\n\n";

  auto s = pluss::make_series({-4.0, 5.0, -6.0});
  auto s_numel = pluss::dispense_as<std::int64_t>(
      s, &pluss::Series::numel, pluss::args{});
  std::cout << "dispense(Series,&Series::numel,args{}): " << s_numel << "\n";

  auto n = pluss::make_note({"apple", "banana", "apricot"});
  pluss::args starts_with_args;
  starts_with_args.emplace_back(std::string_view("a"));
  auto starts_with_a = pluss::dispense_as<pluss::Note>(
      n, &pluss::Note::starts_with, starts_with_args);
  std::cout << "dispense(Note,&Note::starts_with,args{...}): ["
            << static_cast<int>(starts_with_a.bools()[0]) << ", "
            << static_cast<int>(starts_with_a.bools()[1]) << ", "
            << static_cast<int>(starts_with_a.bools()[2]) << "]\n";

  auto t = pluss::timeline::make("2024-01-15T10:11:12Z");
  pluss::args floor_args;
  floor_args.emplace_back(std::string_view("D"));
  auto day_floor = pluss::dispense_as<pluss::Timeline>(
      t, &pluss::Timeline::floor, floor_args);
  std::cout << "dispense(Timeline,&Timeline::floor,args{...})->hour: "
            << day_floor.hour().host<std::int32_t>().array().data.at(static_cast<std::size_t>(0)) << "\n";

  auto year_series = pluss::dispense_as<pluss::Series>(
      t, &pluss::Timeline::year, pluss::args{});
  std::cout << "dispense(Timeline,&Timeline::year,args{}) value="
            << year_series.host<std::int32_t>().array().data.at(static_cast<std::size_t>(0)) << "\n";

  std::cout << "\nDone.\n";
  return 0;
}


// Architecture and subsystem boundaries: autodiff_architecture.md
// Practical contract for gradient-based optimization implementers: optimization_diff_contract.md
