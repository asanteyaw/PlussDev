<!-- generated from growing_the_problem.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Growing the problem

[Back to Choosing and Tuning an Algorithm](../optima_03_choosing_an_algorithm.md)


The cost of a method shows in how its work grows with the number of variables. The problem is a smoothing fit with `n` variables, `2(n - 1)` difference constraints, one equality and bounds, whose solution has many active constraints. Iterations and function evaluations are shown; an asterisk would mark a run that did not converge.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;

  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};

  std::cout << std::left << std::setw(18) << "iters / evals";
  for (int n : {8, 16, 32}) std::cout << std::setw(14) << ("n = " + std::to_string(n));
  std::cout << "\n";

  for (const auto& row : rows) {
    std::cout << std::left << std::setw(18) << row.label;
    for (int n : {8, 16, 32}) {
      std::vector<double> target(n), start(n), lo(n, 0.0), hi(n, 1.0);
      double total = 0.0;
      for (int i = 0; i < n; ++i) {
        target[i] = 0.2 + 0.6 * i / (n - 1) + 0.15 * std::sin(1.7 * i);
        start[i] = (i % 2 == 0) ? 0.9 : 0.1;
        total += target[i];
      }
      std::vector<double> diff(static_cast<std::size_t>(2 * (n - 1) * n), 0.0);
      std::vector<double> cap(static_cast<std::size_t>(2 * (n - 1)), 0.12);
      for (int i = 0; i + 1 < n; ++i) {
        diff[static_cast<std::size_t>((2 * i) * n + i)] = 1.0;
        diff[static_cast<std::size_t>((2 * i) * n + i + 1)] = -1.0;
        diff[static_cast<std::size_t>((2 * i + 1) * n + i)] = -1.0;
        diff[static_cast<std::size_t>((2 * i + 1) * n + i + 1)] = 1.0;
      }
      auto vec = [&](const std::vector<double>& v) {
        return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
      };
      const auto goal = optim::trace(vec(target), false);
      auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - goal)); };
      const auto constraints =
          optim::constraints()
              .ln_ineq(pluss::make_series(diff, {2 * (n - 1), n}).to(f64), vec(cap))
              .ln_eq(pluss::make_series(std::vector<double>(n, 1.0), {1, n}).to(f64), vec({total}))
              .lbs(vec(lo))
              .ubs(vec(hi));
      optim::OptimaOptions options;
      options.algorithm = row.algorithm;
      options.penalty.max_iterations = 2000;
      options.sequential_qp.max_iterations = 500;
      options.working_set.max_iterations = 500;
      options.internal_protocol.max_iterations = 500;
      const auto r = optim::optima(objective, vec(start), options, constraints);
      std::cout << std::setw(14)
                << (std::to_string(r.iteration) + " / " + std::to_string(r.function_evaluations) +
                    (r.exit_flag == optim::OptimaExitFlag::Converged ? "" : " *"));
    }
    std::cout << "\n";
  }
  std::cout << "* did not converge\n";
  return 0;
}
```

Output

```text
iters / evals     n = 8         n = 16        n = 32        
MeritMethod       38 / 67       60 / 119      84 / 196      
SequentialQP      7 / 8         8 / 9         8 / 13        
WorkingSet        7 / 8         12 / 28       11 / 20       
InternalProtocol  12 / 16       15 / 19       14 / 18       
* did not converge
```
Sequential QP, the working-set method and the interior method each need 7 to 15 iterations at every size, and between 8 and 28 evaluations. The merit method needs more iterations as the problem grows (38, 60, 84) and more evaluations (67, 119, 196). When each evaluation is expensive, evaluations are the number to watch: sequential QP is the cheapest at every size (8, 9 and 13), the interior method comes next (16, 19 and 18), and the working-set method is the most expensive at 16 variables (28).
