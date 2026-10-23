<!-- generated from the_line_search.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The line search

[Back to Sequential QP in Depth](../optima_04_sequential_qp.md)


The same problem, with each line-search setting varied on its own.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> start_point() { return optim::pack_scalars({{"x", scalar(-1.9)}, {"y", scalar(2.0)}}); }
// Rosenbrock's curved valley, restricted to the disc x^2 + y^2 <= 1.5 and to the hyperbola x y = 0.4.
auto objective = [](const Pack& p) {
  return 100.0 * optim::square(p.at("y") - optim::square(p.at("x"))) + optim::square(1.0 - p.at("x"));
};
auto constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return optim::square(p.at("x")) + optim::square(p.at("y")) - 1.5; })
      .nl_eq([](const Pack& p) { return p.at("x") * p.at("y") - 0.4; });
}

int main() {
  std::cout << std::left << std::setw(34) << "setting" << std::setw(6) << "flag" << std::setw(7) << "iters"
            << std::setw(7) << "evals" << std::setw(13) << "objective" << "violation\n";
  auto run = [&](const std::string& label, const optim::OptimaOptions& options, const auto& start, const auto& cons, const auto& f) {
    const auto r = optim::optima(f, start, options, cons);
    std::cout << std::left << std::setw(34) << label << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7)
              << r.iteration << std::setw(7) << r.function_evaluations << std::fixed << std::setprecision(5)
              << std::setw(13) << r.evaluation.value.template item<double>() << std::scientific << std::setprecision(1)
              << r.evaluation.max_constraint_violation << std::defaultfloat << "\n";
  };
  for (double b : {0.2, 0.5, 0.8}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.backtracking = b;
    run("backtracking " + std::to_string(b).substr(0, 4), o, start_point(), constraints(), objective);
  }
  for (double a : {1e-6, 1e-4, 1e-1}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.armijo = a;
    std::ostringstream label;
    label << "armijo " << a;
    run(label.str(), o, start_point(), constraints(), objective);
  }
  for (std::int64_t k : {1, 3, 20}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.max_line_search_iterations = k;
    run("max_line_search_iterations " + std::to_string(k), o, start_point(), constraints(), objective);
  }
  for (double s0 : {0.1, 0.5, 1.0}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.initial_step_size = s0;
    run("initial_step_size " + std::to_string(s0).substr(0, 4), o, start_point(), constraints(), objective);
  }
  return 0;
}
```

Output

```text
setting                           flag  iters  evals  objective    violation
backtracking 0.20                 1     11     18     0.06913      3.8e-07
backtracking 0.50                 1     10     14     0.06913      8.8e-10
backtracking 0.80                 1     10     17     0.06913      5.8e-10
armijo 1e-06                      1     10     14     0.06913      8.8e-10
armijo 0.0001                     1     10     14     0.06913      8.8e-10
armijo 0.1                        1     11     21     0.06913      3.3e-09
max_line_search_iterations 1      1     11     16     0.06913      1.3e-09
max_line_search_iterations 3      1     10     14     0.06913      8.8e-10
max_line_search_iterations 20     1     10     14     0.06913      8.8e-10
initial_step_size 0.10            0     100    101    164.54577    4.0e-01
initial_step_size 0.50            -6    55     149    8.14438      7.5e-13
initial_step_size 1.00            1     10     14     0.06913      8.8e-10
```
Three facts stand out. The default backtracking factor of 0.5 is the cheapest here, 14 evaluations against 17 for 0.8 and 18 for 0.2. A loose `armijo` of 0.1 demands so much decrease that 21 evaluations are spent, while 1e-6 and 1e-4 behave exactly like the default. The trial-step budget barely matters on this problem: one trial step per iteration still converges (11 iterations, 16 evaluations), and so do 3 and 20 (10 iterations, 14 evaluations); Program 5 shows a problem where a budget of one fails. The setting that decides whether the run works at all is `initial_step_size`: 0.1 leaves the run unconverged after 100 iterations at objective 164.546, and 0.5 stops with flag `-6` at 8.14438 after 55 iterations. Keep the full initial step: a Newton-type method gets its fast local convergence from it.
