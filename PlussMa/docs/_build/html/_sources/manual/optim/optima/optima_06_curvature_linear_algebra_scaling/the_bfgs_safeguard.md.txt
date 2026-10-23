<!-- generated from the_bfgs_safeguard.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The BFGS safeguard

[Back to Curvature, Linear Algebra and Scaling](../optima_06_curvature_linear_algebra_scaling.md)


A BFGS update is only valid when the step and the change in gradient point the same way (the curvature condition). A pair that fails it is skipped, and `bfgs_curvature_tolerance` sets how strict that test is.

```cpp
#include <pluss/ma.h>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;
using Block = optim::ParameterPack<pluss::Series>::Block;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

// The Rosenbrock chain: n variables, strongly coupled curvature, boxed to [-3, 3], started at (-1.2, 1, -1.2, ...).
struct Chain {
  int n;
  std::string name(int i) const { return "x" + std::to_string(i); }
  optim::ParameterPack<pluss::Series> filled(double lo_or_hi) const {
    std::vector<Block> b;
    for (int i = 0; i < n; ++i) b.push_back({name(i), scalar(lo_or_hi).reshape({1})});
    return optim::ParameterPack<pluss::Series>(b);
  }
  optim::ParameterPack<pluss::Series> start() const {
    std::vector<Block> b;
    for (int i = 0; i < n; ++i) b.push_back({name(i), scalar(i % 2 == 0 ? -1.2 : 1.0).reshape({1})});
    return optim::ParameterPack<pluss::Series>(b);
  }
  optim::Trace objective(const Pack& p) const {
    optim::Trace total = p.at(name(0)) * 0.0;
    for (int i = 0; i + 1 < n; ++i)
      total = total + 100.0 * optim::square(p.at(name(i + 1)) - optim::square(p.at(name(i)))) + optim::square(1.0 - p.at(name(i)));
    return total;
  }
  auto box() const { return optim::constraints().lbs(filled(-3.0)).ubs(filled(3.0)); }
  optim::OptimaState solve(const optim::OptimaOptions& o) const {
    return optim::optima([this](const Pack& p) { return objective(p); }, start(), o, box());
  }
};

int main() {
  std::cout << std::left << std::setw(12) << "tolerance" << std::setw(18) << "algorithm" << std::setw(6) << "flag" << std::setw(7)
            << "iters" << std::setw(7) << "evals" << "objective\n";
  for (double tol : {1e-12, 1e-8, 1e-4, 1e-1, 1.0}) {
    for (const auto alg : {optim::OptimaAlgorithm::SequentialQP, optim::OptimaAlgorithm::InternalProtocol}) {
      optim::OptimaOptions o;
      o.algorithm = alg;
      o.sequential_qp.bfgs_curvature_tolerance = tol;
      o.internal_protocol.bfgs_curvature_tolerance = tol;
      o.sequential_qp.max_iterations = 300;
      o.internal_protocol.max_iterations = 300;
      const auto r = Chain{12}.solve(o);
      std::ostringstream t;
      t << tol;
      std::cout << std::left << std::setw(12) << t.str() << std::setw(18)
                << (alg == optim::OptimaAlgorithm::SequentialQP ? "SequentialQP" : "InternalProtocol") << std::setw(6)
                << static_cast<int>(r.exit_flag) << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations
                << std::scientific << std::setprecision(2) << r.evaluation.value.template item<double>() << "\n";
    }
  }
  return 0;
}
```

Output

```text
tolerance   algorithm         flag  iters  evals  objective
1e-12       SequentialQP      1     85     97     5.46e-10
1e-12       InternalProtocol  1     88     102    5.22e-09
1e-08       SequentialQP      1     85     97     5.46e-10
1e-08       InternalProtocol  1     88     102    5.22e-09
0.0001      SequentialQP      1     85     97     5.46e-10
0.0001      InternalProtocol  1     88     102    5.22e-09
0.1         SequentialQP      1     89     100    6.26e-09
0.1         InternalProtocol  1     93     107    4.32e-08
1           SequentialQP      0     300    301    1.10e+01
1           InternalProtocol  0     300    302    1.10e+01
```
From 1e-12 to 1e-4 the result is identical: the default is far below anything that matters. At 0.1 the cost shows (89 and 93 iterations against 85 and 88), and at 1 neither algorithm converges in 300 iterations, with an objective of 11.0 left. Do not raise this tolerance to "make the model more cautious": it makes the model stop learning.
