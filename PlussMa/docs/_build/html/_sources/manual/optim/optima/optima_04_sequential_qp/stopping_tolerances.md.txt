<!-- generated from stopping_tolerances.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Stopping tolerances

[Back to Sequential QP in Depth](../optima_04_sequential_qp.md)


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
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}
auto objective = [](const Pack& p) { return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c"); };
auto constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
      .nl_eq([](const Pack& p) {
        return optim::square(p.at("a")) + optim::square(p.at("b")) + optim::square(p.at("c")) + optim::square(p.at("d")) - 40.0;
      })
      .lbs(four(1, 1, 1, 1))
      .ubs(four(5, 5, 5, 5));
}

int main() {
  for (double tol : {1e-3, 1e-6, 1e-9}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.optimality_tolerance = tol;
    o.sequential_qp.constraint_tolerance = tol;
    const auto r = optim::optima(objective, four(1, 5, 5, 1), o, constraints());
    std::cout << std::scientific << std::setprecision(0) << "tolerances " << tol << ": " << std::fixed << std::setprecision(8)
              << r.evaluation.value.template item<double>() << ", " << r.iteration << " iterations, \""
              << r.message.substr(0, r.message.find('\n')) << "\"\n";
  }
  return 0;
}
```

Output

```text
tolerances 1e-03: 17.01371683, 3 iterations, "optimality and feasibility tolerances satisfied"
tolerances 1e-06: 17.01401725, 4 iterations, "optimality and feasibility tolerances satisfied"
tolerances 1e-09: 17.01401729, 5 iterations, "predicted objective improvement below tolerance at a feasible point"
```
Tightening the optimality and constraint tolerances together from 1e-3 to 1e-9 moves the objective from 17.01371683 to 17.01401729 and the iteration count from 3 to 5. The tightest run stops on "predicted objective improvement below tolerance at a feasible point" rather than on the optimality test: at 1e-9 the remaining improvement is below what the objective can resolve, which is the sign to stop asking for more precision. Ask for the precision your decision needs, not more: the three runs agree to four decimals.
