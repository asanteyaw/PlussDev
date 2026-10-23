<!-- generated from how_the_model_scales_with_the_problem.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# How the model scales with the problem

[Back to Curvature, Linear Algebra and Scaling](../optima_06_curvature_linear_algebra_scaling.md)


The Rosenbrock chain, with its strongly coupled curvature, is solved for 6, 12 and 24 variables. Each cell is iterations and function evaluations.

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
  struct Col { const char* label; optim::OptimaAlgorithm alg; optim::OptimaHessianApproximation model; };
  const Col cols[] = {
      {"SequentialQP Bfgs", optim::OptimaAlgorithm::SequentialQP, optim::OptimaHessianApproximation::Bfgs},
      {"Interior Bfgs", optim::OptimaAlgorithm::InternalProtocol, optim::OptimaHessianApproximation::Bfgs},
      {"Interior Lbfgs", optim::OptimaAlgorithm::InternalProtocol, optim::OptimaHessianApproximation::Lbfgs},
      {"Interior Identity", optim::OptimaAlgorithm::InternalProtocol, optim::OptimaHessianApproximation::Identity}};
  std::cout << std::left << std::setw(8) << "n";
  for (const auto& c : cols) std::cout << std::setw(22) << c.label;
  std::cout << "\n";
  for (int n : {6, 12, 24}) {
    std::cout << std::left << std::setw(8) << n;
    for (const auto& c : cols) {
      optim::OptimaOptions o;
      o.algorithm = c.alg;
      o.sequential_qp.hessian_approximation = c.model;
      o.internal_protocol.hessian_approximation = c.model;
      o.sequential_qp.max_iterations = 300;
      o.internal_protocol.max_iterations = 300;
      const auto r = Chain{n}.solve(o);
      std::cout << std::setw(22)
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
n       SequentialQP Bfgs     Interior Bfgs         Interior Lbfgs        Interior Identity     
6       56 / 61               57 / 65               51 / 64               300 / 302 *           
12      85 / 97               88 / 102              80 / 91               300 / 302 *           
24      145 / 162             145 / 155             137 / 161             300 / 302 *           
* did not converge
```
The real models grow gently: sequential QP with BFGS needs 56, 85 and 145 iterations; the interior method with BFGS 57, 88 and 145; with the limited-memory model 51, 80 and 137. The limited-memory model is the cheapest in iterations here, not just in storage. The identity model, which throws curvature away, does not converge in 300 iterations at any size. Curvature information is the difference between a method that finishes and one that crawls; the choice between BFGS and its limited-memory version is a smaller matter that depends on size and storage.
