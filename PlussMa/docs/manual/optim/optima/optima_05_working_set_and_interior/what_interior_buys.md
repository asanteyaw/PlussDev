<!-- generated from what_interior_buys.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# What "interior" buys

[Back to The Working-Set and Interior Methods](../optima_05_working_set_and_interior.md)


Bounds and linear rows are kept strictly satisfied at every iterate of the interior method. The program minimizes a function that contains a logarithm, so it is defined only for positive arguments, from a start far from the minimum, and records the smallest value any iterate takes.

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

int main() {
  auto f = [](const optim::Trace& x) { return optim::sum(x) - 0.1 * optim::sum(optim::log(x)); };
  const auto cons = optim::constraints().lbs(vec({0.0, 0.0})).ubs(vec({10.0, 10.0}));
  const std::pair<const char*, optim::OptimaAlgorithm> algs[] = {
      {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}, {"SequentialQP", optim::OptimaAlgorithm::SequentialQP}};
  std::cout << std::left << std::setw(18) << "algorithm" << std::setw(6) << "flag" << std::setw(7) << "iters"
            << std::setw(24) << "smallest value of x" << "objective\n";
  for (const auto& [name, alg] : algs) {
    optim::OptimaOptions o;
    o.algorithm = alg;
    double smallest = 1e300;
    o.progress_hook = [&](const optim::OptimaProgressSnapshot& s) {
      for (double v : s.params.at("x").to_vec<double>()) smallest = std::min(smallest, v);
      return false;
    };
    const auto r = optim::optima(f, vec({9.0, 9.0}), o, cons);
    std::cout << std::left << std::setw(18) << name << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7) << r.iteration
              << std::scientific << std::setprecision(2) << std::setw(24) << smallest << std::fixed << std::setprecision(6)
              << r.evaluation.value.template item<double>() << "\n";
  }
  return 0;
}
```

Output

```text
algorithm         flag  iters  smallest value of x     objective
InternalProtocol  1     11     3.06e-02                0.660517
SequentialQP      -6    3      5.40e-12                5.189051
```
The interior method converges in 11 iterations to 0.660517, and its smallest iterate is 3.06e-02: it never reaches the boundary. Sequential QP projects its steps onto the bounds and lands on `x = 0` to within rounding (5.40e-12), where the logarithm is huge. Its line search then fails (flag `-6`, objective 5.189051). If your objective is undefined outside the feasible region, this is the reason to use the interior method with `independent_nonlinear_slacks = false` for nonlinear inequalities as well (chapter 3, Program 2).
