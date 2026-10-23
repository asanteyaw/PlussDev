<!-- generated from the_penalty_weight.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The penalty weight

[Back to Sequential QP in Depth](../optima_04_sequential_qp.md)


A weight that is too small lets iterates drift off the constraints; a weight that is too large makes every step look like a loss of feasibility. The program starts from a curved valley (Rosenbrock's function with an inequality and an equality constraint) at an infeasible point, with `initial_penalty` from 0.1 to 1000.

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
  for (double w : {0.1, 1.0, 10.0, 100.0, 1000.0}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.initial_penalty = w;
    run("initial_penalty " + std::to_string(w).substr(0, 7), o, start_point(), constraints(), objective);
  }
  return 0;
}
```

Output

```text
setting                           flag  iters  evals  objective    violation
initial_penalty 0.10000           1     9      14     0.06913      3.5e-07
initial_penalty 1.00000           1     9      14     0.06913      3.5e-07
initial_penalty 10.0000           1     10     14     0.06913      8.8e-10
initial_penalty 100.000           1     10     22     0.06913      5.0e-10
initial_penalty 1000.00           1     12     34     0.06913      1.1e-09
```
All five rows reach the same objective, 0.06913, and the three smallest weights cost the same 14 evaluations. A weight of 100 costs 22 and a weight of 1000 costs 34: a weight that is too large makes every step look like a loss of feasibility, so the line search backtracks more before it accepts one. The solver adapts the weight itself, so a poor starting guess is corrected within a few iterations and costs evaluations, not the answer. Leave the default; if a run fails, look at the line search before the penalty.
