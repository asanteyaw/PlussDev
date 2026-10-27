<!-- generated from the_working_set_method.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The working-set method

[Back to Optima 5: The Working-Set and Interior Methods](../optima_05_working_set_and_interior.md)

A working-set (active-set) method treats the inequalities as two groups at each iteration: the *active* ones, which it enforces as equalities, and the rest, which it ignores until a step threatens to violate one. The reason usually given to prefer it is that most constraints are inactive at a solution, so following only the active ones should take less work. This page watches the method do what it says, and then tests the claim by counting.

The first program prints the iteration trace of a six-variable problem with six linear rows, of which two matter, and reads the answer off the multipliers: positive multipliers mark the active set. The second adds hundreds of inactive rows and compares the work of all three quadratic-model methods, which tests whether the "ignore the inactive rows" idea pays in this implementation. The third sweeps the working-set method's own options to show which ones matter.

## Program 1: Watching the active set

The problem has six variables, a quartic objective, and six linear inequality rows. Two of them matter: the sum of the variables is at most 6 and the first variable is at most 0.5. The other four are never active. The program prints the iteration trace and then the multipliers of the six rows.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

// min sum (x_j - 2)^4 + 0.1 x_j^2 over 6 variables (not quadratic, so every iteration is a genuine step), with `extra` linear inequalities on top of two that matter:
// sum x <= 6 and x_0 <= 0.5. The extra rows have generous right-hand sides and are never active.
struct Rows { pluss::Series A, b; };
Rows make_rows(int extra) {
  const int n = 6, m = 2 + extra;
  std::vector<double> A(static_cast<std::size_t>(m * n), 0.0), b(static_cast<std::size_t>(m), 0.0);
  for (int j = 0; j < n; ++j) A[j] = 1.0;
  b[0] = 6.0;
  A[n + 0] = 1.0;
  b[1] = 0.5;
  for (int k = 0; k < extra; ++k) {
    double l1 = 0.0;
    for (int j = 0; j < n; ++j) {
      const double a = std::sin(1.7 * (k + 1) + 0.9 * (j + 1));
      A[(2 + k) * n + j] = a;
      l1 += std::abs(a);
    }
    b[2 + k] = 3.0 * l1;
  }
  return {pluss::make_series(A, {m, n}).to(f64), pluss::make_series(b, {m}).to(f64)};
}
auto objective = [](const pluss::Series& x) { return (x - 2.0).pow(4.0).sum() + 0.1 * x.square().sum(); };

int main() {
  const auto rows = make_rows(4);
  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::WorkingSet;
  std::cout << std::right << std::setw(5) << "iter" << std::setw(12) << "step" << std::setw(14) << "stationarity"
            << std::setw(12) << "violation" << std::setw(12) << "objective\n";
  options.progress_hook = [](const optim::OptimaProgressSnapshot& s) {
    if (s.stage != optim::OptimaProgressStage::Iteration) return false;
    std::cout << std::setw(5) << s.iteration << std::scientific << std::setprecision(2) << std::setw(12) << s.step_size
              << std::setw(14) << s.stationarity << std::setw(12) << s.evaluation.max_constraint_violation << std::fixed
              << std::setprecision(5) << std::setw(12) << s.evaluation.value.template item<double>() << "\n";
    return false;
  };
  const auto r = optim::optima(objective, vec({0, 0, 0, 0, 0, 0}), options, optim::constraints().ln_ineq(rows.A, rows.b));

  std::cout << "\nexit flag " << static_cast<int>(r.exit_flag) << ": " << r.message.substr(0, r.message.find('\n')) << "\n";
  const auto x = r.params.at("x").host<double>();
  std::cout << std::fixed << std::setprecision(4) << "x = [";
  for (double v : x) std::cout << " " << v;
  std::cout << " ]\nmultipliers of the 6 inequality rows:";
  for (double v : r.multipliers.linear_inequalities.host<double>()) std::cout << " " << v;
  std::cout << "\n";
  return 0;
}
```

Output

```text
 iter        step  stationarity   violation  objective
    1    1.00e+00      3.44e-02    0.00e+00     8.97300
    1    1.00e+00      4.69e-07    0.00e+00     8.97300

exit flag 1: working set satisfies optimality and feasibility tolerances
x = [ 0.5000 1.1000 1.1000 1.1000 1.1000 1.1000 ]
multipliers of the 6 inequality rows: 2.6960 10.7040 0.0000 0.0000 0.0000 0.0000
```
The multipliers are the active set: rows 1 and 2 have positive multipliers (2.6960 and 10.7040) and rows 3 to 6 have zero. The answer sits on both active constraints, with the first variable at 0.5000 and the other five at 1.1000, where the constraint on their sum is tight. The last iteration appears twice, with stationarity 3.44e-02 and then 4.69e-07: the second line is the same point after the multipliers have been refreshed, and it is what satisfies the tolerance.

## Program 2: Does the active list pay off?

The reason usually given to prefer a working-set method is that most constraints are inactive, so following only the active ones should take less work. This program tests that claim by counting. The same problem is solved with `extra` inactive rows added, and each algorithm's iterations and function evaluations are reported.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

// min sum (x_j - 2)^4 + 0.1 x_j^2 over 6 variables (not quadratic, so every iteration is a genuine step), with `extra` linear inequalities on top of two that matter:
// sum x <= 6 and x_0 <= 0.5. The extra rows have generous right-hand sides and are never active.
struct Rows { pluss::Series A, b; };
Rows make_rows(int extra) {
  const int n = 6, m = 2 + extra;
  std::vector<double> A(static_cast<std::size_t>(m * n), 0.0), b(static_cast<std::size_t>(m), 0.0);
  for (int j = 0; j < n; ++j) A[j] = 1.0;
  b[0] = 6.0;
  A[n + 0] = 1.0;
  b[1] = 0.5;
  for (int k = 0; k < extra; ++k) {
    double l1 = 0.0;
    for (int j = 0; j < n; ++j) {
      const double a = std::sin(1.7 * (k + 1) + 0.9 * (j + 1));
      A[(2 + k) * n + j] = a;
      l1 += std::abs(a);
    }
    b[2 + k] = 3.0 * l1;
  }
  return {pluss::make_series(A, {m, n}).to(f64), pluss::make_series(b, {m}).to(f64)};
}
auto objective = [](const pluss::Series& x) { return (x - 2.0).pow(4.0).sum() + 0.1 * x.square().sum(); };

int main() {
  const std::pair<const char*, optim::OptimaAlgorithm> algs[] = {
      {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
      {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
      {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};
  std::cout << std::left << std::setw(8) << "extra" << std::setw(18) << "algorithm" << std::setw(6) << "flag" << std::setw(7)
            << "iters" << std::setw(7) << "evals" << std::setw(12) << "objective\n";
  for (int extra : {0, 100, 500, 2000}) {
    const auto rows = make_rows(extra);
    for (const auto& [name, alg] : algs) {
      optim::OptimaOptions o;
      o.algorithm = alg;
      const auto r = optim::optima(objective, vec({0, 0, 0, 0, 0, 0}), o, optim::constraints().ln_ineq(rows.A, rows.b));
      std::cout << std::left << std::setw(8) << extra << std::setw(18) << name << std::setw(6) << static_cast<int>(r.exit_flag)
                << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations << std::fixed << std::setprecision(6)
                << r.evaluation.value.template item<double>() << "\n";
    }
  }
  return 0;
}
```

Output

```text
extra   algorithm         flag  iters  evals  objective
  0       SequentialQP      1     1      2      8.973002
0       WorkingSet        1     1      2      8.973002
0       InternalProtocol  1     7      9      8.973001
100     SequentialQP      1     2      3      8.973000
100     WorkingSet        1     2      3      8.973000
100     InternalProtocol  1     13     17     8.973000
500     SequentialQP      1     2      3      8.973000
500     WorkingSet        1     4      5      8.973000
500     InternalProtocol  1     15     21     8.973000
2000    SequentialQP      1     2      3      8.973000
2000    WorkingSet        1     4      5      8.973000
2000    InternalProtocol  1     14     20     8.973000
```
In this implementation the claim does not hold. All three methods find the same answer, and sequential QP needs the fewest iterations and evaluations at every size. The working-set method follows sequential QP's path in the same number of iterations at 0 and 100 extra rows (1 and 2) and in twice as many at 500 and 2000 (4 against 2),, because maintaining the list has a cost of its own. The interior method takes more iterations (7 at no extra rows against 1) but its count barely grows: 7, 13, 15 and 14 across the table, against 1, 2, 2 and 2 for sequential QP. Plan on the working-set method for what it returns, an explicit active set, not for a smaller iteration count.

## Program 3: The working-set options

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

// min sum (x_j - 2)^4 + 0.1 x_j^2 over 6 variables (not quadratic, so every iteration is a genuine step), with `extra` linear inequalities on top of two that matter:
// sum x <= 6 and x_0 <= 0.5. The extra rows have generous right-hand sides and are never active.
struct Rows { pluss::Series A, b; };
Rows make_rows(int extra) {
  const int n = 6, m = 2 + extra;
  std::vector<double> A(static_cast<std::size_t>(m * n), 0.0), b(static_cast<std::size_t>(m), 0.0);
  for (int j = 0; j < n; ++j) A[j] = 1.0;
  b[0] = 6.0;
  A[n + 0] = 1.0;
  b[1] = 0.5;
  for (int k = 0; k < extra; ++k) {
    double l1 = 0.0;
    for (int j = 0; j < n; ++j) {
      const double a = std::sin(1.7 * (k + 1) + 0.9 * (j + 1));
      A[(2 + k) * n + j] = a;
      l1 += std::abs(a);
    }
    b[2 + k] = 3.0 * l1;
  }
  return {pluss::make_series(A, {m, n}).to(f64), pluss::make_series(b, {m}).to(f64)};
}
auto objective = [](const pluss::Series& x) { return (x - 2.0).pow(4.0).sum() + 0.1 * x.square().sum(); };

int main() {
  const auto rows = make_rows(500);
  std::cout << std::left << std::setw(46) << "setting" << std::setw(6) << "flag" << std::setw(7) << "iters" << std::setw(7)
            << "evals" << "objective\n";
  auto run = [&](const std::string& label, optim::OptimaOptions o) {
    o.algorithm = optim::OptimaAlgorithm::WorkingSet;
    const auto r = optim::optima(objective, vec({0, 0, 0, 0, 0, 0}), o, optim::constraints().ln_ineq(rows.A, rows.b));
    std::cout << std::left << std::setw(46) << label << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7)
              << r.iteration << std::setw(7) << r.function_evaluations << std::fixed << std::setprecision(6)
              << r.evaluation.value.template item<double>() << "\n";
  };
  run("defaults", {});
  for (double t : {1e-6, 1e-1, 1.0}) {
    optim::OptimaOptions o;
    o.working_set.add_constraint_tolerance = t;
    run("add_constraint_tolerance " + std::to_string(t).substr(0, 8), o);
  }
  for (double t : {1e-12, 1e-4, 1e-1}) {
    optim::OptimaOptions o;
    o.working_set.drop_multiplier_tolerance = t;
    run("drop_multiplier_tolerance " + std::to_string(t).substr(0, 8), o);
  }
  {
    optim::OptimaOptions o;
    o.working_set.favor_working_set_updates = false;
    run("favor_working_set_updates false", o);
  }
  for (std::int64_t k : {1, 2, 8}) {
    optim::OptimaOptions o;
    o.working_set.max_working_set_violations = k;
    run("max_working_set_violations " + std::to_string(k), o);
  }
  return 0;
}
```

Output

```text
setting                                       flag  iters  evals  objective
defaults                                      1     4      5      8.973000
add_constraint_tolerance 0.000001             1     4      5      8.973000
add_constraint_tolerance 0.100000             1     4      5      8.973000
add_constraint_tolerance 1.000000             1     4      5      8.973000
drop_multiplier_tolerance 0.000000            1     4      5      8.973000
drop_multiplier_tolerance 0.000100            1     4      5      8.973000
drop_multiplier_tolerance 0.100000            1     4      5      8.973000
favor_working_set_updates false               1     6      6      8.973006
max_working_set_violations 1                  1     4      5      8.973000
max_working_set_violations 2                  1     4      5      8.973000
max_working_set_violations 8                  1     4      5      8.973000
```
None of `add_constraint_tolerance`, `drop_multiplier_tolerance` or `max_working_set_violations` changed the run here: 4 iterations and 5 evaluations in every row. Only `favor_working_set_updates` mattered, and in the wrong direction: switching it off took 6 iterations. The defaults are sound for a well-posed problem. These options are for the degenerate cases, where a constraint hovers at its limit and is added and dropped repeatedly (a stall the method reports itself, and the `max_stalled_iterations` setting caps).

## Decisions and pitfalls

- **The multipliers are the active set.** Rows with a positive multiplier are active at the answer; zero means inactive. Read them to see which constraints shape the solution.
- **Test the folklore on your problem.** Here the working-set method did not beat SQP as inactive rows were added; maintaining the active list has its own cost. Choose it for degenerate or highly structured problems, not because "most constraints are inactive".
- **Leave the working-set options at their defaults.** None of the tolerances changed the run; the options are for degenerate cases, and switching `favor_working_set_updates` off cost iterations.
- **The last iteration may be reported twice.** The `done` snapshot repeats the final iterate with its refreshed stationarity; do not read it as an extra iteration.

Where next: [The interior-point method](the_interior_point_method.md) is the opposite strategy: stay strictly inside and let a barrier do the work.
