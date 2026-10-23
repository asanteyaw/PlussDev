<!-- generated from prices_checked_by_re_solving.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Prices, checked by re-solving

[Back to Prices and Certificates](../linprog_02_prices_and_certificates.md)


The workshop of chapter 1. For each machine the program prints the hours left over at the optimum, the dual value the solver reports, the price in profit (the negated dual value, since the program minimizes the negated profit), and the price found independently by re-solving with the hours changed by plus and minus 0.01 and dividing the change in profit by 0.02.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <cmath>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

lp::Problem workshop(std::vector<double> hours) {
  lp::Problem p;
  p.c = vec({-3.0, -5.0});
  p.a_ub = mat({1, 0, 0, 2, 3, 2}, 3, 2);
  p.b_ub = vec(hours);
  p.lower = vec({0.0, 0.0});
  return p;
}

// Round-off of an interior-point answer can print as -0.000; show it as zero.
double clean(double v) { return std::abs(v) < 5e-4 ? 0.0 : v; }

int main() {
  const std::vector<double> hours = {4.0, 12.0, 18.0};
  const auto base = lp::solve(workshop(hours));
  const auto x = base.x.to_vec<double>();
  const auto dual = base.dual_ub.to_vec<double>();
  const std::vector<double> used = {x[0], 2.0 * x[1], 3.0 * x[0] + 2.0 * x[1]};

  std::cout << std::fixed << std::setprecision(3);
  std::cout << "optimal profit " << -base.objective.item<double>() << " at (" << x[0] << ", " << x[1] << ")\n\n";
  std::cout << std::left << std::setw(11) << "machine" << std::setw(12) << "idle hours" << std::setw(14) << "dual value" << std::setw(16) << "price in profit" << "price by re-solving\n";
  const char* names[] = {"A", "B", "C"};
  for (std::size_t i = 0; i < 3; ++i) {
    auto more = hours, fewer = hours;
    more[i] += 0.01;
    fewer[i] -= 0.01;
    const double up = -lp::solve(workshop(more)).objective.item<double>();
    const double down = -lp::solve(workshop(fewer)).objective.item<double>();
    std::cout << std::left << std::setw(11) << names[i] << std::setw(12) << hours[i] - used[i] << std::setw(14) << clean(dual[i]) << std::setw(16) << clean(-dual[i]) << clean((up - down) / 0.02) << "\n";
  }
  std::cout << "\nstrong duality: primal objective " << base.objective.item<double>() << ", dual objective " << base.dual_objective.item<double>() << "\n";
  return 0;
}
```

Output

```text
optimal profit 36.000 at (2.000, 6.000)

machine    idle hours  dual value    price in profit price by re-solving
A          2.000       0.000         0.000           0.000
B          0.000       -1.500        1.500           1.500
C          0.000       -1.000        1.000           1.000

strong duality: primal objective -36.000, dual objective -36.000
```
Machine A has 2 idle hours and a price of 0: another hour of A would not earn anything. Machine B is fully used and an extra hour is worth 1.5 in profit; machine C is fully used and an hour is worth 1.0. The re-solved prices agree with the reported dual values to the printed digits, so the duals are not to be taken on trust; they can be checked, and this check costs only two extra solves per row. The last line is strong duality: the primal and dual objectives are both -36.

The prices say where to spend effort. An hour of machine B is worth half as much again as an hour of machine C, and an hour of machine A is worth nothing, so if hours can be bought or moved, the order of interest is B, then C, never A. They also say how far the statement holds: a price is a rate at the current plan, and once enough hours are added that a different set of machines binds, the rate changes.
