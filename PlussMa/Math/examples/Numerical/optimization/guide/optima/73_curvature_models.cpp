// All three algorithms accept all four curvature models. `Bfgs` and `Lbfgs` build the model from the steps taken;
// `Exact` and `HessianVector` compute the true Hessian of the Lagrangian (the objective plus the multiplier-weighted
// constraints) by differentiating the problem's own functions a second time, so they need no extra callbacks: each
// function is written once, over plain Series. The problem is the four-variable benchmark with one nonlinear
// inequality and one nonlinear equality; every cell reaches the same optimum, 17.0140173, and the table shows what
// each model costs to get there. The last lines show the one refusal that remains: an exact model on an objective
// given only as a value and a gradient, which cannot be differentiated again.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}

pluss::Series objective_of(const optim::ParameterPack<pluss::Series>& p) {
  return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c");
}
pluss::Series inequality_of(const optim::ParameterPack<pluss::Series>& p) {
  return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d");
}
pluss::Series equality_of(const optim::ParameterPack<pluss::Series>& p) {
  return p.at("a").square() + p.at("b").square() + p.at("c").square() + p.at("d").square() - 40.0;
}

optim::ConstrainedProblem problem(bool objective_as_series) {
  optim::ConstrainedProblem prob;
  if (objective_as_series) {
    prob.objective.autodiff = objective_of;
  } else {
    prob.objective.analytic = [](const optim::ParameterPack<pluss::Series>& p) {
      const double a = as_f64(p.at("a")), b = as_f64(p.at("b")), c = as_f64(p.at("c")), d = as_f64(p.at("d"));
      return std::pair{scalar(a * d * (a + b + c) + c), four(d * (2 * a + b + c), a * d, a * d + 1.0, a * (a + b + c))};
    };
  }
  prob.nonlinear.inequality = inequality_of;
  prob.nonlinear.equality = equality_of;
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
  std::cout << "Exact on a value-and-gradient objective: exit flag " << static_cast<int>(refused.exit_flag) << "\n"
            << refused.message.substr(0, refused.message.find(" diagnostic")) << "\n";
  return 0;
}
