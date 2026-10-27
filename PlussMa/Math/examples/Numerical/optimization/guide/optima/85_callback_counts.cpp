// What each way of supplying derivatives costs, counted in calls to your own functions. The four-variable benchmark is
// solved by sequential QP with a quasi-Newton curvature model (BFGS), with the exact Hessian, and with a hand-written
// objective gradient. Every call into the objective callbacks is counted: `traced` calls record the function on the
// tape (the exact Hessian differentiates it again, so those calls are counted here too), and `analytic` calls return
// the value and gradient directly.
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

struct Counts { int traced = 0, analytic = 0; };

optim::ConstrainedProblem problem(Counts& counts, bool analytic_gradient) {
  optim::ConstrainedProblem prob;
  if (analytic_gradient) {
    prob.objective.analytic = [&counts](const optim::ParameterPack<pluss::Series>& p) {
      ++counts.analytic;
      const double a = as_f64(p.at("a")), b = as_f64(p.at("b")), c = as_f64(p.at("c")), d = as_f64(p.at("d"));
      return std::pair{scalar(a * d * (a + b + c) + c), four(d * (2 * a + b + c), a * d, a * d + 1.0, a * (a + b + c))};
    };
  } else {
    prob.objective.autodiff = [&counts](const optim::ParameterPack<pluss::Series>& p) { ++counts.traced; return objective_of(p); };
  }
  prob.nonlinear.inequality = [](const optim::ParameterPack<pluss::Series>& p) { return inequality_of(p); };
  prob.nonlinear.equality = [](const optim::ParameterPack<pluss::Series>& p) { return equality_of(p); };
  prob.bounds.lower = four(1, 1, 1, 1);
  prob.bounds.upper = four(5, 5, 5, 5);
  return prob;
}

int main() {
  struct Row { const char* label; bool analytic; optim::OptimaHessianApproximation model; };
  const Row rows[] = {{"traced gradient, BFGS", false, optim::OptimaHessianApproximation::Bfgs},
                      {"analytic gradient, BFGS", true, optim::OptimaHessianApproximation::Bfgs},
                      {"traced gradient, exact Hessian", false, optim::OptimaHessianApproximation::Exact}};
  std::cout << std::left << std::setw(34) << "setup" << std::setw(12) << "iterations" << std::setw(10) << "traced" << std::setw(10) << "analytic" << "objective\n";
  for (const auto& row : rows) {
    Counts counts;
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.hessian_approximation = row.model;
    const auto r = optim::optima(problem(counts, row.analytic), four(1, 5, 5, 1), o);
    std::cout << std::left << std::setw(34) << row.label << std::setw(12) << r.iteration << std::setw(10) << counts.traced
              << std::setw(10) << counts.analytic << std::fixed << std::setprecision(7) << r.evaluation.value.item<double>() << "\n";
  }
  return 0;
}
