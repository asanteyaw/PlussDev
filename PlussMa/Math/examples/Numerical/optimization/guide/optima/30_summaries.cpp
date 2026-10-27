// Three ready-made reports for a finished run. compact_summary (and print_summary, its alias) is one
// line for logs. detailed_summary is one field per line for a terminal.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  const auto r = optim::optima([](const pluss::Series& x) { return (x - 3.0).square().sum(); },
                               vec({0.0, 0.0}), {}, optim::constraints().ubs(vec({2.0, 2.5})));

  std::cout << "-- compact_summary --\n";
  optim::reports::compact_summary(std::cout, r) << "\n";
  std::cout << "-- print_summary (same line) --\n";
  optim::print_summary(std::cout, r) << "\n";
  std::cout << "-- detailed_summary --\n";
  optim::reports::detailed_summary(std::cout, r) << "\n";
  return 0;
}
