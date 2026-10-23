<!-- generated from does_the_active_list_pay_off.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Does the active list pay off?

[Back to The Working-Set and Interior Methods](../optima_05_working_set_and_interior.md)


The reason usually given to prefer a working-set method is that most constraints are inactive, so following only the active ones should take less work. This program tests that claim by counting. The same problem is solved with `extra` inactive rows added, and each algorithm's iterations and function evaluations are reported.

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
