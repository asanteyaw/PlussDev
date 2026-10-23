<!-- generated from the_working_set_options.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The working-set options

[Back to The Working-Set and Interior Methods](../optima_05_working_set_and_interior.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;
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
auto objective = [](const optim::Trace& x) { return optim::sum(optim::pow(x - 2.0, 4.0)) + 0.1 * optim::sum(optim::square(x)); };

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
