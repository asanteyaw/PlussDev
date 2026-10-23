<!-- generated from counting_the_calls.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Counting the calls

[Back to Derivatives](../optima_07_derivatives.md)


Each way of supplying derivatives costs a different number of calls into your own functions. The four-variable benchmark is solved by sequential QP with the BFGS model and a traced gradient, with the BFGS model and a hand-written gradient, and with the exact Hessian. Every call into the objective callbacks is counted.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}

template <class AD>
AD objective_of(const optim::ParameterPack<AD>& p) {
  return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c");
}
template <class AD>
AD inequality_of(const optim::ParameterPack<AD>& p) {
  return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d");
}
template <class AD>
AD equality_of(const optim::ParameterPack<AD>& p) {
  return optim::square(p.at("a")) + optim::square(p.at("b")) + optim::square(p.at("c")) + optim::square(p.at("d")) - 40.0;
}

struct Counts { int traced = 0, dual = 0, analytic = 0; };

optim::ConstrainedProblem problem(Counts& counts, bool analytic_gradient, bool dual) {
  optim::ConstrainedProblem prob;
  if (analytic_gradient) {
    prob.objective.analytic = [&counts](const optim::ParameterPack<pluss::Series>& p) {
      ++counts.analytic;
      const double a = p.at("a").item<double>(), b = p.at("b").item<double>(), c = p.at("c").item<double>(), d = p.at("d").item<double>();
      return std::pair{scalar(a * d * (a + b + c) + c), four(d * (2 * a + b + c), a * d, a * d + 1.0, a * (a + b + c))};
    };
  } else {
    prob.objective.autodiff = [&counts](const optim::ParameterPack<optim::Trace>& p) { ++counts.traced; return objective_of<optim::Trace>(p); };
  }
  if (dual) prob.objective.autodiff_dual = [&counts](const optim::ParameterPack<optim::DualTrace>& p) { ++counts.dual; return objective_of<optim::DualTrace>(p); };
  prob.nonlinear.inequality = [](const optim::ParameterPack<optim::Trace>& p) { return inequality_of<optim::Trace>(p); };
  prob.nonlinear.equality = [](const optim::ParameterPack<optim::Trace>& p) { return equality_of<optim::Trace>(p); };
  prob.nonlinear.inequality_dual = [](const optim::ParameterPack<optim::DualTrace>& p) { return inequality_of<optim::DualTrace>(p); };
  prob.nonlinear.equality_dual = [](const optim::ParameterPack<optim::DualTrace>& p) { return equality_of<optim::DualTrace>(p); };
  prob.bounds.lower = four(1, 1, 1, 1);
  prob.bounds.upper = four(5, 5, 5, 5);
  return prob;
}

int main() {
  struct Row { const char* label; bool analytic; bool dual; optim::OptimaHessianApproximation model; };
  const Row rows[] = {{"traced gradient, BFGS", false, false, optim::OptimaHessianApproximation::Bfgs},
                      {"analytic gradient, BFGS", true, false, optim::OptimaHessianApproximation::Bfgs},
                      {"traced gradient, exact Hessian", false, true, optim::OptimaHessianApproximation::Exact}};
  std::cout << std::left << std::setw(34) << "setup" << std::setw(12) << "iterations" << std::setw(10) << "traced" << std::setw(8)
            << "dual" << std::setw(10) << "analytic" << "objective\n";
  for (const auto& row : rows) {
    Counts counts;
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.hessian_approximation = row.model;
    const auto r = optim::optima(problem(counts, row.analytic, row.dual), four(1, 5, 5, 1), o);
    std::cout << std::left << std::setw(34) << row.label << std::setw(12) << r.iteration << std::setw(10) << counts.traced << std::setw(8)
              << counts.dual << std::setw(10) << counts.analytic << std::fixed << std::setprecision(7) << r.evaluation.value.item<double>() << "\n";
  }
  return 0;
}
```

Output

```text
setup                             iterations  traced    dual    analytic  objective
traced gradient, BFGS             4           6         0       0         17.0140172
analytic gradient, BFGS           4           0         0       6         17.0140172
traced gradient, exact Hessian    5           7         24      0         17.0140173
```
With BFGS the run takes 4 iterations and 6 calls to the objective, whether the gradient is traced or written by hand; the hand-written one appears in the `analytic` column instead, with the same count and the same answer. The exact model reaches the same optimum in 5 iterations and 7 traced calls, and adds 24 calls to the dual callback. That is 6 Hessians of 4 variables each: a Hessian is assembled from one Hessian-vector product per variable. The number of dual calls grows with the problem as `n` per iteration, which is the cost of exactness (chapter 6); a function whose evaluation is cheap, or whose curvature is learned slowly by the quasi-Newton models, can repay it. A function whose evaluation is expensive is better served by BFGS or the limited-memory model.
