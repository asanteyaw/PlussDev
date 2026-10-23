// All three algorithms accept all four curvature models. `Bfgs` and `Lbfgs` build the model from the steps taken;
// `Exact` and `HessianVector` compute the true Hessian of the Lagrangian (the objective plus the multiplier-weighted
// constraints) by forward-over-reverse differentiation, which needs the problem's dual-number callbacks: every
// function is written once, generically, and handed over twice (`autodiff` for gradients, `autodiff_dual` for
// Hessian-vector products). The problem is the four-variable benchmark with one nonlinear inequality and one nonlinear
// equality; every cell reaches the same optimum, 17.0140173, and the table shows what each model costs to get there.
// The last lines show the one refusal that remains: an exact model without dual callbacks.
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
