<!-- generated from prices_and_certificates.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Prices and certificates

[Back to Linear Programming 2: Prices and Certificates](../linprog_02_prices_and_certificates.md)

Every constraint of a linear program has a **price**: the amount by which the optimum would improve if its limit were relaxed by one unit. The dual values are those prices, and they say where to spend effort: a fully used machine with a high price is worth expanding, a machine with idle hours and price zero is not. The first program checks the prices *without trusting the solver*, by re-solving with one hour more of each machine and comparing the change in the objective with the reported dual value.

The second program is about proofs. A verdict of infeasible or unbounded is only as good as its certificate, and the certificate is in the result: for infeasibility, Farkas multipliers that combine the rows into a contradiction; for unboundedness, a ray along which every row stays satisfied and the cost falls. Both can be verified in a few lines, which turns "the solver says infeasible" into a statement you can show someone.

## Program 1: Prices, checked by re-solving

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
  const auto x = base.x.host<double>();
  const auto dual = base.dual_ub.host<double>();
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

## Program 2: Certificates you can check

A verdict of infeasible or unbounded is only as good as its proof, and the proof is in the result.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <limits>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  std::cout << std::fixed << std::setprecision(4) << std::boolalpha;
  {
    // x0 + x1 <= 1 and x0 + x1 >= 3 with x >= 0
    lp::Problem p;
    p.c = vec({1, 1});
    p.a_ub = mat({1, 1, -1, -1}, 2, 2);
    p.b_ub = vec({1, -3});
    p.lower = vec({0, 0});
    const auto r = lp::solve(p);
    const auto y = r.farkas.host<double>();  // one multiplier per inequality row
    const double g0 = y[0] * 1 + y[1] * -1, g1 = y[0] * 1 + y[1] * -1;  // g = A' y
    const double b_dot_y = y[0] * 1 + y[1] * -3;
    // with x >= 0 and no upper bound, g . x is largest at x = 0 if every component of g is <= 0
    const double largest = (g0 <= 0 && g1 <= 0) ? 0.0 : std::numeric_limits<double>::infinity();
    std::cout << "infeasible program: g = A'y = (" << g0 << ", " << g1 << "), largest g . x on x >= 0 is " << largest << ", b . y = " << b_dot_y << "\n";
    std::cout << "  every feasible point would need g . x >= b . y, but g . x <= " << largest << " < " << b_dot_y << ", so there is none: " << (largest < b_dot_y) << "\n\n";
  }
  {
    // minimize -x0 with x1 <= 1 and x >= 0: x0 can grow without limit
    lp::Problem p;
    p.c = vec({-1, 0});
    p.a_ub = mat({0, 1}, 1, 2);
    p.b_ub = vec({1});
    p.lower = vec({0, 0});
    const auto r = lp::solve(p);
    const auto d = r.ray.host<double>();
    const double along_row = 0.0 * d[0] + 1.0 * d[1];  // A d must be <= 0
    const double cost_change = -1.0 * d[0] + 0.0 * d[1];  // c . d must be < 0
    std::cout << "unbounded program: ray d = (" << d[0] << ", " << d[1] << ")\n";
    std::cout << "  A d = " << along_row << " (must be <= 0), c . d = " << cost_change << " (must be < 0), d >= 0 so the bounds are respected\n";
    std::cout << "  the ray is a valid direction of unbounded descent: " << (along_row <= 1e-6 && cost_change < 0 && d[0] >= -1e-6 && d[1] >= -1e-6) << "\n";
  }
  return 0;
}
```

Output

```text
infeasible program: g = A'y = (-0.7956, -0.7956), largest g . x on x >= 0 is 0.0000, b . y = 0.9088
  every feasible point would need g . x >= b . y, but g . x <= 0.0000 < 0.9088, so there is none: true

unbounded program: ray d = (1.5830, 0.0000)
  A d = 0.0000 (must be <= 0), c . d = -1.5830 (must be < 0), d >= 0 so the bounds are respected
  the ray is a valid direction of unbounded descent: true
```
**Infeasible.** The program `x0 + x1 <= 1`, `x0 + x1 >= 3`, `x >= 0` cannot be satisfied. The Farkas multipliers `y` are weights on the rows, each zero or negative for a `<=` row. Multiply each row by its weight and add them. Because the weights are negative, each inequality flips, and the sum says that every point satisfying the rows has `g . x >= b . y`, where `g = A'y`. Here `g = (-0.7956, -0.7956)` and `b . y = 0.9088`. On `x >= 0` the largest `g . x` can be is 0, which is below 0.9088. So any feasible point would need `g . x >= 0.9088` while the bounds keep `g . x <= 0`: there is none. The program checks the inequality with arithmetic that does not call the solver, and it holds.

**Unbounded.** The program "minimize `-x0` with `x1 <= 1`, `x >= 0`" can lower its objective forever by growing `x0`. The ray `d = (1.583, 0)` is a direction along which every row stays satisfied (`A d = 0 <= 0`), the bounds are respected (`d >= 0`) and the cost falls (`c . d = -1.583 < 0`). Any feasible point plus any multiple of `d` is feasible with a lower cost. The scale of a ray or of a set of multipliers is arbitrary; only the direction and the signs matter.

These checks are worth running in your own code when a verdict surprises you, and they turn "the solver says infeasible" into a statement you can show someone.

## Decisions and pitfalls

- **A price is valid only locally.** It is the first-order value of relaxing a limit; it holds while the set of binding constraints does not change, and says nothing about large changes.
- **A fully used resource has a positive price; one with idle hours has zero.** The order of interest in the workshop is machine B, then C, never A.
- **Check surprising verdicts.** If the solver says infeasible or unbounded and you do not expect it, run the certificate checks; they usually point at a sign or a bound in the model.
- **Prices measure the objective's units.** The dual of a program that minimises negated profit is the negated price in profit; mind the sign.

Where next: [Chapter 3](../linprog_03_methods_warm_starts_and_sweeps.md) is about using the methods well.
