<!-- generated from every_algorithm_every_model.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Every algorithm, every model

[Back to Curvature, Linear Algebra and Scaling](../optima_06_curvature_linear_algebra_scaling.md)


All three algorithms accept all four models. The exact models need the problem's dual-number callbacks: each function is written once, generically, and handed over twice, `autodiff` for the gradient and `autodiff_dual` for Hessian-vector products. The problem is the four-variable benchmark with one nonlinear inequality and one nonlinear equality.

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

// The same text serves both differentiation modes: AD is optim::Trace or optim::DualTrace.
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

optim::ConstrainedProblem problem(bool with_dual_callbacks) {
  optim::ConstrainedProblem prob;
  prob.objective.autodiff = [](const optim::ParameterPack<optim::Trace>& p) { return objective_of<optim::Trace>(p); };
  prob.nonlinear.inequality = [](const optim::ParameterPack<optim::Trace>& p) { return inequality_of<optim::Trace>(p); };
  prob.nonlinear.equality = [](const optim::ParameterPack<optim::Trace>& p) { return equality_of<optim::Trace>(p); };
  if (with_dual_callbacks) {
    prob.objective.autodiff_dual = [](const optim::ParameterPack<optim::DualTrace>& p) { return objective_of<optim::DualTrace>(p); };
    prob.nonlinear.inequality_dual = [](const optim::ParameterPack<optim::DualTrace>& p) { return inequality_of<optim::DualTrace>(p); };
    prob.nonlinear.equality_dual = [](const optim::ParameterPack<optim::DualTrace>& p) { return equality_of<optim::DualTrace>(p); };
  }
  prob.bounds.lower = four(1, 1, 1, 1);
  prob.bounds.upper = four(5, 5, 5, 5);
  return prob;
}

int main() {
  struct Alg { const char* label; optim::OptimaAlgorithm alg; };
  struct Model { const char* label; optim::OptimaHessianApproximation model; };
  const Alg algs[] = {{"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                      {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                      {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};
  const Model models[] = {{"Bfgs", optim::OptimaHessianApproximation::Bfgs},
                          {"Lbfgs", optim::OptimaHessianApproximation::Lbfgs},
                          {"Exact", optim::OptimaHessianApproximation::Exact},
                          {"HessianVector", optim::OptimaHessianApproximation::HessianVector}};

  std::cout << "cell = iterations / evaluations    objective\n";
  std::cout << std::left << std::setw(18) << "algorithm";
  for (const auto& m : models) std::cout << std::setw(16) << m.label;
  std::cout << "\n";
  double worst_gap = 0.0;
  for (const auto& a : algs) {
    std::cout << std::left << std::setw(18) << a.label;
    for (const auto& m : models) {
      optim::OptimaOptions o;
      o.algorithm = a.alg;
      o.sequential_qp.hessian_approximation = m.model;
      o.working_set.hessian_approximation = m.model;
      o.internal_protocol.hessian_approximation = m.model;
      const auto r = optim::optima(problem(true), four(1, 5, 5, 1), o);
      worst_gap = std::max(worst_gap, std::abs(r.evaluation.value.template item<double>() - 17.0140172891));
      std::cout << std::setw(16) << (std::to_string(r.iteration) + " / " + std::to_string(r.function_evaluations));
    }
    std::cout << "\n";
  }
  std::cout << "largest distance of any cell from the optimum: " << std::scientific << std::setprecision(1) << worst_gap << "\n\n";

  optim::OptimaOptions exact;
  exact.algorithm = optim::OptimaAlgorithm::SequentialQP;
  exact.sequential_qp.hessian_approximation = optim::OptimaHessianApproximation::Exact;
  const auto refused = optim::optima(problem(false), four(1, 5, 5, 1), exact);
  std::cout << "Exact without dual callbacks: exit flag " << static_cast<int>(refused.exit_flag) << "\n"
            << refused.message.substr(0, refused.message.find(" diagnostic")) << "\n";
  return 0;
}
```

Output

```text
cell = iterations / evaluations    objective
algorithm         Bfgs            Lbfgs           Exact           HessianVector   
SequentialQP      4 / 5           6 / 8           5 / 6           5 / 6           
WorkingSet        4 / 6           6 / 9           5 / 7           5 / 7           
InternalProtocol  8 / 12          9 / 13          14 / 33         14 / 33         
largest distance of any cell from the optimum: 6.2e-07

Exact without dual callbacks: exit flag -5
exact curvature requires objective.autodiff_dual and dual nonlinear callbacks (hessian_approximation is Exact or HessianVector)
```
All twelve cells reach the optimum of the problem, 17.0140173, to within 6.2e-07 of it. The cheapest in evaluations is the dense BFGS model, which on this small problem needs 5, 6 and 12 evaluations in the three algorithms; the exact models are close in the first two (6 and 7) and more expensive in the interior method (33). On this small problem the cheap quasi-Newton models are as good as the exact ones, and the table does not show an advantage for the exact models. They give the true curvature at once instead of learning it from the steps, and they cost a Hessian-vector product per variable per iteration plus the dual callbacks, so they pay off where the quasi-Newton model learns slowly or where the Hessian is cheap to evaluate.

The last two lines show the one remaining refusal. An exact model without dual callbacks is rejected with exit flag `-5` and a message that names the missing callbacks, rather than quietly using another model, so a run never reports results for a model it did not use. The exact Hessian is made positive definite where it has to be: the shift is chosen so that the curvature is positive on the directions the active constraints leave free, not on the whole space, so a full Newton step is kept whenever that reduced curvature is already positive.
