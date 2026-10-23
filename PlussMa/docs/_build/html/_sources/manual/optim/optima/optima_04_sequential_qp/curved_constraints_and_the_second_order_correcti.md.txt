<!-- generated from curved_constraints_and_the_second_order_correcti.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Curved constraints and the second-order correction

[Back to Sequential QP in Depth](../optima_04_sequential_qp.md)


Near the solution of a curved equality constraint, the best step of the linearized model can raise both the objective and the violation of the true problem, so a good step is rejected (the Maratos effect). A second-order correction re-solves the subproblem with the constraint measured at the trial point, which cancels the curvature error.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <cmath>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}

int main() {
  const auto start = optim::pack_scalars({{"x", scalar(std::cos(0.9))}, {"y", scalar(std::sin(0.9))}});
  auto f = [](const Pack& p) { return 2.0 * (optim::square(p.at("x")) + optim::square(p.at("y")) - 1.0) - p.at("x"); };
  const auto cons = optim::constraints().nl_eq([](const Pack& p) {
    return optim::square(p.at("x")) + optim::square(p.at("y")) - 1.0;
  });
  std::cout << std::left << std::setw(30) << "max_second_order_corrections" << std::setw(7) << "iters" << std::setw(7)
            << "evals" << std::setw(12) << "objective" << "x\n";
  for (std::int64_t k : {0, 1, 2}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.max_second_order_corrections = k;
    const auto r = optim::optima(f, start, o, cons);
    std::cout << std::left << std::setw(30) << k << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations
              << std::fixed << std::setprecision(6) << std::setw(12) << r.evaluation.value.item<double>()
              << r.params.at("x").item<double>() << "\n";
  }
  return 0;
}
```

Output

```text
max_second_order_corrections  iters  evals  objective   x
0                             8      18     -1.000000   1.000000
1                             7      18     -1.000000   1.000000
2                             7      18     -1.000000   1.000000
```
All three runs reach the minimum, `-1` at `x = 1`. With the correction off the run takes 8 iterations and 18 evaluations; with one or two corrections it takes 7 iterations and the same 18 evaluations. The saving is one iteration, because the problem is small and the starting point is close to the solution, but the cause is exactly the one above, and the correction is cheap insurance on any problem with curved equality constraints.
